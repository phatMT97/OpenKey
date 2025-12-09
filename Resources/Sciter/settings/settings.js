// OpenKey Settings JavaScript
// Handles div-based toggles with hidden inputs for VALUE_CHANGED events

document.on("ready", function () {
    console.log("Settings dialog ready");
    initializeToggles();
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
            console.log("Toggle clicked:", id, "New state:", newState);
            
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

// Handle dropdown changes (already works via C++ VALUE_CHANGED handler)
document.on("change", "select", function (evt, select) {
    const id = select.id || select.getAttribute("id");
    const value = parseInt(select.value);
    console.log("Dropdown changed:", id, "value:", value);
});

// Handle text input changes
document.on("change", "#switch-key-char", function (evt, input) {
    const char = input.value.toUpperCase();
    if (char.length > 0) {
        input.value = char.charAt(0);
        console.log("Switch key char changed:", char.charCodeAt(0));
    }
});

// Handle button clicks
document.on("click", "button", function (evt, button) {
    const id = button.id || button.getAttribute("id");
    console.log("Button clicked:", id);
});
