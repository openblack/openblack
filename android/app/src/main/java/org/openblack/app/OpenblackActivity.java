package org.openblack.app;

import android.Manifest;
import android.content.ContextWrapper;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;

import androidx.annotation.RequiresApi;

import org.libsdl.app.SDLActivity;

public class OpenblackActivity extends SDLActivity {

    /**
     * This method returns the name of the application entry point
     * It can be overridden by derived classes.
     */
    protected String getMainFunction() {
        return "main";
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
        ContextWrapper c = new ContextWrapper(this);
        if (c.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {

            // Should we show an explanation?
            if (shouldShowRequestPermissionRationale(
                    Manifest.permission.READ_EXTERNAL_STORAGE)) {
                // Explain to the user why we need to read the contacts
            }

            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    0x13648559);
        }
        // FIXME(bwrsandman): Currently requires user to upload game assets to "/data/local/tmp/bw" and `chmod -R a+wrx /data/local/tmp/bw`
        return new String[]{
                "--game-path", "/data/local/tmp/bw",
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
