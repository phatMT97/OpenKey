// OpenKey Settings JavaScript
// Handles div-based toggles with hidden inputs for VALUE_CHANGED events

document.on("ready", function () {

    initializeToggles();
    initializeAdvancedPanel();
});

function initializeToggles() {
    // Get all toggle elements and attach handlers
    const allToggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");

    allToggles.forEach(function (toggle) {
        toggle.onclick = function (evt) {
            const isChecked = this.classList.contains("checked");

            // Toggle the visual state
            if (isChecked) {
                this.classList.remove("checked");
            } else {
                this.classList.add("checked");
            }

            const id = this.id;
            const newState = !isChecked;


            // Update the hidden input to fire VALUE_CHANGED
            const hiddenInput = document.getElementById("val-" + id);
            if (hiddenInput) {
                hiddenInput.value = newState ? "1" : "0";
                // Trigger change event for C++ to catch
                hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
            }

            return true;
        };
    });
}

// Initialize advanced panel toggle and tabs
function initializeAdvancedPanel() {


    // Tab switching
    const tabItems = document.querySelectorAll(".tab-item");
    tabItems.forEach(function (tab) {
        tab.onclick = function () {
            switchTab(this.getAttribute("data-tab"));
            return true;
        };
    });

}

// Handle btn-advanced click via event delegation (more reliable in Sciter)
document.on("click", "#btn-advanced", function (evt, el) {

    toggleAdvancedSettings();
    return true;
});

// Toggle advanced settings panel expansion
function toggleAdvancedSettings() {

    const container = document.getElementById("main-container");
    if (container) {
        const isExpanded = container.classList.contains("expanded");

        if (isExpanded) {
            container.classList.remove("expanded");

        } else {
            container.classList.add("expanded");

        }

        // IMMEDIATE synchronous force reflow on toggles
        // Do it as fast as possible to minimize visible flash
        const toggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");
        toggles.forEach(function (toggle) {
            toggle.style.display = "none";
        });
        // Force synchronous reflow by reading offsetHeight
        container.offsetHeight;
        toggles.forEach(function (toggle) {
            toggle.style.display = "";
        });

        // Notify C++ to resize window
        const hiddenInput = document.getElementById("val-expand-state");
        if (hiddenInput) {
            hiddenInput.value = !isExpanded ? "1" : "0";
            hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
        }
    }
}

// Switch between tabs
function switchTab(tabIndex) {


    // Update active tab item
    const tabItems = document.querySelectorAll(".tab-item");
    tabItems.forEach(function (tab) {
        if (tab.getAttribute("data-tab") === tabIndex) {
            tab.classList.add("active");
        } else {
            tab.classList.remove("active");
        }
    });

    // Update active tab panel
    const tabPanels = document.querySelectorAll(".tab-panel");
    tabPanels.forEach(function (panel) {
        const panelIndex = panel.id.replace("tab-panel-", "");
        if (panelIndex === tabIndex) {
            panel.classList.add("active");
        } else {
            panel.classList.remove("active");
        }
    });
}

// Handle dropdown changes (already works via C++ VALUE_CHANGED handler)
document.on("change", "select", function (evt, select) {
    const id = select.id || select.getAttribute("id");
    const value = parseInt(select.value);

});

// Handle text input changes
document.on("change", "#switch-key-char", function (evt, input) {
    const char = input.value.toUpperCase();
    if (char.length > 0) {
        input.value = char.charAt(0);

    }
});

// Handle button clicks
document.on("click", "button", function (evt, button) {
    const id = button.id || button.getAttribute("id");

});
