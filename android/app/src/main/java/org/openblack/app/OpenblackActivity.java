package org.openblack.app;

import android.Manifest;
import android.app.Activity;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.UriPermission;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import org.libsdl.app.SDLActivity;

import java.io.IOException;
import java.util.Arrays;

public class OpenblackActivity extends SDLActivity {

    private static final int REQUEST_GAME_PATH = 1001;
    private static final String PREFERENCES = "Openblack";
    private static final String GAME_PATH_KEY = "selected_directory_uri";
    private boolean gamePathRequestDone;
    @Nullable
    private String pickedGamePath;

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_GAME_PATH) {
            Uri uri = resultCode == Activity.RESULT_OK && data != null ? data.getData() : null;
            if (uri != null) {
                // Persist the grant so the folder stays readable after restarts
                getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                getSharedPreferences(PREFERENCES, MODE_PRIVATE)
                        .edit()
                        .putString(GAME_PATH_KEY, uri.toString())
                        .apply();
            } else {
                Log.e("OpenblackActivity", "Directory not selected or error");
            }

            // Wake the SDL thread even when cancelled, otherwise it waits forever
            synchronized (this) {
                pickedGamePath = uri != null ? uri.toString() : null;
                gamePathRequestDone = true;
                notifyAll();
            }
        }
    }

    /**
     * This method returns the name of the application entry point
     * It can be overridden by derived classes.
     */
    protected String getMainFunction() {
        return "main";
    }

    /**
     * Returns the previously chosen game directory if its read grant is still held.
     */
    @Nullable
    private String getSavedGamePath() {
        String saved = getSharedPreferences(PREFERENCES, MODE_PRIVATE).getString(GAME_PATH_KEY, null);
        if (saved == null) {
            return null;
        }
        Uri uri = Uri.parse(saved);
        for (UriPermission permission : getContentResolver().getPersistedUriPermissions()) {
            if (permission.getUri().equals(uri) && permission.isReadPermission()) {
                return saved;
            }
        }
        Log.w("OpenblackActivity", "Lost access to saved game directory, asking again");
        return null;
    }

    @Nullable
    private String requestGamePath() {
        String saved = getSavedGamePath();
        if (saved != null) {
            return saved;
        }

        synchronized (this) {
            gamePathRequestDone = false;
            pickedGamePath = null;
        }
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_GAME_PATH);

        // Block the SDL thread until the user picks or cancels
        synchronized (this) {
            while (!gamePathRequestDone) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Log.e("OpenblackActivity", "Error waiting for game path selection", e);
                    Thread.currentThread().interrupt();
                    break;
                }
            }
            return pickedGamePath;
        }
    }

    /**
     * This method is called by SDL before starting the native application thread.
     * It can be overridden to provide the arguments after the application name.
     * The default implementation returns an empty array. It never returns null.
     *
     * @return arguments for the native application.
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    protected String[] getArguments() {
        String gamePath = requestGamePath();
        if (gamePath == null) {
            return new String[]{"--backend-type", "Vulkan", "--log-file", "logcat"};
        }
        return new String[]{
                "--game-path", gamePath,
                "--backend-type", "Vulkan",
                "--log-file", "logcat"
        };
    }

    /**
     * This method is called by SDL before loading the native shared libraries.
     * It can be overridden to provide names of shared libraries to be loaded.
     * The default implementation returns the defaults. It never returns null.
     * An array returned by a new implementation must at least contain "SDL2".
     * Also keep in mind that the order the libraries are loaded may matter.
     *
     * @return names of shared libraries to be loaded (e.g. "SDL2", "main").
     */
    protected String[] getLibraries() {
        return new String[]{
                "openblack_lib"
        };
    }
}
