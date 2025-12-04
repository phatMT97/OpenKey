// About Dialog JavaScript
// Handles UI interactions and C++ communication via SOM_PASSPORT

document.ready = function () {
    console.log("About dialog initialized");

    // Request version info from C++
    requestVersionInfo();

    // Setup event listeners
    setupLinkHandlers();
    setupButtonHandlers();
};

// Request version information from C++
function requestVersionInfo() {
    // This will be called by C++ after window creation
    // Window.this.setVersionInfo(version, buildDate) will be called from C++
}

// Handle external links - prevent Sciter from loading them internally
function setupLinkHandlers() {
    document.on('click', 'a[data-link]', function (evt) {
        evt.preventDefault();

        const url = this.getAttribute('href');
        console.log("Opening external URL:", url);

        // Call C++ function to open in system browser
        if (Window.this && Window.this.openUrl) {
            Window.this.openUrl(url);
        } else {
            console.error("openUrl function not available");
        }

        return true;
    });
}

// Setup button event handlers
function setupButtonHandlers() {
    const updateBtn = document.getElementById('check-update');
    const closeBtn = document.getElementById('close-btn');

    // Update check button
    if (updateBtn) {
        updateBtn.onclick = function () {
            console.log("Check update clicked");

            // Disable button and show loading state
            updateBtn.disabled = true;
            updateBtn.classList.add('loading');

            // Call C++ function to check for updates
            if (Window.this && Window.this.checkUpdate) {
                Window.this.checkUpdate();
            } else {
                console.error("checkUpdate function not available");
                updateBtn.disabled = false;
                updateBtn.classList.remove('loading');
            }
        };
    }

    // Close button
    if (closeBtn) {
        closeBtn.onclick = function () {
            console.log("Close clicked");

            if (Window.this && Window.this.closeWindow) {
                Window.this.closeWindow();
            } else {
                console.error("closeWindow function not available");
            }
        };
    }
}

// Called from C++ to set version information
function setVersionInfo(version, buildDate) {
    console.log("Setting version info:", version, buildDate);

    const versionEl = document.getElementById('version');
    const buildDateEl = document.getElementById('build-date');

    if (versionEl) {
        versionEl.textContent = version;
    }

    if (buildDateEl) {
        buildDateEl.textContent = buildDate;
    }
}

// Called from C++ to show update check result
function showUpdateResult(hasUpdate, newVersion) {
    console.log("Update result:", hasUpdate, newVersion);

    const updateBtn = document.getElementById('check-update');

    // Re-enable button
    if (updateBtn) {
        updateBtn.disabled = false;
        updateBtn.classList.remove('loading');
    }

    if (hasUpdate) {
        // Show update available message
        const message = `OpenKey có phiên bản mới (${newVersion}), bạn có muốn cập nhật không?`;

        if (Window.this && Window.this.showUpdateDialog) {
            Window.this.showUpdateDialog(message, newVersion);
        } else {
            console.log("Update available:", newVersion);
        }
    } else {
        // Show already up-to-date message
        if (Window.this && Window.this.showInfoMessage) {
            Window.this.showInfoMessage("Bạn đang dùng phiên bản mới nhất!");
        } else {
            console.log("Already up to date");
        }
    }
}

// Export functions for C++ to call
globalThis.setVersionInfo = setVersionInfo;
globalThis.showUpdateResult = showUpdateResult;
