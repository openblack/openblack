package org.openblack.app;

import android.Manifest;
import android.app.Activity;
import android.content.ContextWrapper;
import android.content.Intent;
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

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_GAME_PATH) {
            if (resultCode == Activity.RESULT_OK && data != null) {
                Uri uri = data.getData();
                if (uri != null) {
                    getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);

                    // Save the URI into SharedPreferences
                    getSharedPreferences("Openblack", MODE_PRIVATE)
                            .edit()
                            .putString("selected_directory_uri", uri.toString())
                            .apply();

                    synchronized (this) {
                        notify();
                    }
                }
            } else {
                Log.e("OpenblackActivity", "Directory not selected or error");
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

    private String requestGamePath() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        startActivityForResult(intent, REQUEST_GAME_PATH);

        // Block execution until the user selects a directory
        synchronized (this) {
            try {
                wait();
            } catch (InterruptedException e) {
                Log.e("OpenblackActivity", "Error waiting for game path selection", e);
            }
        }

        return getSharedPreferences("Openblack", MODE_PRIVATE).getString("selected_directory_uri", null);
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
