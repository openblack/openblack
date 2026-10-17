package org.openblack.app;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Native file access to the game directory picked through the Storage Access Framework.
 *
 * Every Storage Access Framework query is an IPC round trip to the documents provider, so each directory is listed
 * once with all the columns needed and kept in memory. The game data is read-only, so listings never go stale.
 */
public class FileSystemInterop {
    private static final String TAG = "FileReader";

    private static final String[] CHILD_PROJECTION = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
    };

    private static final class Entry {
        final String name;
        final String documentId;
        final boolean isDirectory;

        Entry(String name, String documentId, boolean isDirectory) {
            this.name = name;
            this.documentId = documentId;
            this.isDirectory = isDirectory;
        }
    }

    private static final class Listing {
        final List<Entry> entries = new ArrayList<>();
        final Map<String, Entry> byName = new HashMap<>();
        // Game data comes from Windows, where file names are case-insensitive
        final Map<String, Entry> byLowerName = new HashMap<>();

        Entry find(String name) {
            Entry entry = byName.get(name);
            return entry != null ? entry : byLowerName.get(name.toLowerCase(Locale.ROOT));
        }
    }

    // Keyed by tree URI then document ID
    private static final Map<String, Listing> listings = new ConcurrentHashMap<>();

    private static Listing getListing(ContentResolver resolver, Uri treeUri, String documentId) {
        String key = treeUri + "\n" + documentId;
        Listing cached = listings.get(key);
        if (cached != null) {
            return cached;
        }

        Listing listing = new Listing();
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, documentId);
        try (Cursor cursor = resolver.query(childrenUri, CHILD_PROJECTION, null, null, null)) {
            if (cursor == null) {
                Log.e(TAG, "Could not list directory: " + childrenUri);
                return null;
            }
            while (cursor.moveToNext()) {
                String name = cursor.getString(1);
                if (name == null) continue;
                boolean isDirectory = DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2));
                Entry entry = new Entry(name, cursor.getString(0), isDirectory);
                listing.entries.add(entry);
                listing.byName.put(name, entry);
                String lowerName = name.toLowerCase(Locale.ROOT);
                if (!listing.byLowerName.containsKey(lowerName)) {
                    listing.byLowerName.put(lowerName, entry);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error listing directory: " + childrenUri, e);
            return null;
        }

        listings.put(key, listing);
        return listing;
    }

    /** Returns the document ID for a path relative to the tree root, or null if it does not exist. */
    private static String resolveDocumentId(ContentResolver resolver, Uri treeUri, String path) {
        return resolveDocumentId(resolver, treeUri, path, true);
    }

    private static String resolveDocumentId(ContentResolver resolver, Uri treeUri, String path, boolean logMissing) {
        String documentId = DocumentsContract.getTreeDocumentId(treeUri);
        for (String part : path.split("/")) {
            if (part.isEmpty() || part.equals(".")) continue;

            Listing listing = getListing(resolver, treeUri, documentId);
            Entry entry = listing != null ? listing.find(part) : null;
            if (entry == null) {
                if (logMissing) {
                    Log.e(TAG, "Path not found: " + path + " (at " + part + ")");
                }
                return null;
            }
            documentId = entry.documentId;
        }
        return documentId;
    }

    private static Uri getDirectoryFromPath(Context context, String storageUriString, String path) {
        Uri treeUri = Uri.parse(storageUriString);
        try {
            String documentId = resolveDocumentId(context.getContentResolver(), treeUri, path);
            return documentId != null ? DocumentsContract.buildDocumentUriUsingTree(treeUri, documentId) : null;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Not a document tree: " + storageUriString, e);
            return null;
        }
    }

    public static boolean existsFromPath(Context context, String storageUriString, String path) {
        try {
            return resolveDocumentId(context.getContentResolver(), Uri.parse(storageUriString), path, false) != null;
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    public static String[] listFilesFromPath(Context context, String storageUriString, String path, boolean recursive) {
        ContentResolver resolver = context.getContentResolver();
        Uri treeUri = Uri.parse(storageUriString);
        ArrayList<String> fileNames = new ArrayList<>();
        try {
            String documentId = resolveDocumentId(resolver, treeUri, path);
            if (documentId != null) {
                listFilesRecursive(resolver, treeUri, documentId, fileNames, recursive, "");
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Not a document tree: " + storageUriString, e);
        }
        return fileNames.toArray(new String[0]);
    }

    private static void listFilesRecursive(ContentResolver resolver, Uri treeUri, String documentId,
                                           ArrayList<String> fileNames, boolean recursive, String path) {
        Listing listing = getListing(resolver, treeUri, documentId);
        if (listing == null) return;

        for (Entry entry : listing.entries) {
            String entryPath = path.isEmpty() ? entry.name : path + "/" + entry.name;
            if (!entry.isDirectory) {
                fileNames.add(entryPath);
            } else if (recursive) {
                listFilesRecursive(resolver, treeUri, entry.documentId, fileNames, true, entryPath);
            }
        }
    }

    /** Returns the whole file, or null if it cannot be found or read. */
    public static byte[] readFileFromPath(Context context, String storageUriString, String path) {
        Uri uri = getDirectoryFromPath(context, storageUriString, path);
        if (uri == null) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        try (ParcelFileDescriptor descriptor = context.getContentResolver().openFileDescriptor(uri, "r")) {
            if (descriptor == null) {
                Log.e(TAG, "Could not open file: " + path);
                return null;
            }
            // available() is only an estimate, so size the buffer from the descriptor and read until full
            long size = descriptor.getStatSize();
            try (InputStream input = new ParcelFileDescriptor.AutoCloseInputStream(descriptor.dup())) {
                return size >= 0 && size <= Integer.MAX_VALUE ? readExactly(input, (int) size) : readToEnd(input);
            }
        } catch (IOException | RuntimeException e) {
            Log.e(TAG, "Error reading file: " + path, e);
        }
        return null;
    }

    private static byte[] readExactly(InputStream input, int size) throws IOException {
        byte[] bytes = new byte[size];
        int offset = 0;
        while (offset < size) {
            int read = input.read(bytes, offset, size - offset);
            if (read < 0) {
                throw new IOException("Unexpected end of file after " + offset + " of " + size + " bytes");
            }
            offset += read;
        }
        return bytes;
    }

    private static byte[] readToEnd(InputStream input) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[64 * 1024];
        int read;
        while ((read = input.read(buffer)) >= 0) {
            output.write(buffer, 0, read);
        }
        return output.toByteArray();
    }
}
