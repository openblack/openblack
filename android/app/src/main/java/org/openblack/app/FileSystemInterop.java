package org.openblack.app;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import androidx.documentfile.provider.DocumentFile;

import java.io.InputStream;
import java.util.Arrays;
import java.util.List;

public class FileSystemInterop {

    private static Uri getDirectoryFromPath(Context context, String storageUriString, String path) {
        Uri uri = Uri.parse(storageUriString);
        DocumentFile directory = DocumentFile.fromTreeUri(context, uri);
        if (directory != null) {
            for (String part : path.split("/")) {
                // Ignore "." paths
                if (part.equals(".")) continue;

                directory = directory.findFile(part);
                if (directory == null || !directory.exists()) {
                    Log.e("FileReader", "Directory not found: " + part);
                    return null;
                }
            }
            return directory.getUri();
        } else {
            Log.e("FileReader", "Directory not found: " + storageUriString);
        }
        return null;
    }

    public static String[] listFilesFromPath(Context context, String storageUriString, String path) {
        Uri uri = getDirectoryFromPath(context, storageUriString, path);
        DocumentFile directory = DocumentFile.fromTreeUri(context, uri);
        DocumentFile[] files = directory.listFiles();
        String[] fileNames = new String[files.length];

        for (int i = 0; i < files.length; i++) {
            fileNames[i] = files[i].getName();
        }

        return fileNames;
    }

    public static byte[] readFileFromPath(Context context, String storageUriString, String path) {
        Uri uri = getDirectoryFromPath(context, storageUriString, path);
        if (uri != null) {
            try {
                InputStream inputStream = context.getContentResolver().openInputStream(uri);
                if (inputStream != null) {
                    byte[] bytes = new byte[inputStream.available()];
                    inputStream.read(bytes);
                    inputStream.close();
                    return bytes;
                } else {
                    Log.e("FileReader", "Input stream is null");
                }
            } catch (Exception e) {
                Log.e("FileReader", "Error reading file", e);
            }
        } else {
            Log.e("FileReader", "File not found: " + storageUriString);
        }
        return new byte[0];
    }
}
