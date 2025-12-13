// Excluded Apps Dialog JavaScript

document.ready = function () {
    initExcludedAppsDialog();
};

function initExcludedAppsDialog() {
    // Bind button clicks
    var btnAddManual = document.getElementById("btn-add-manual");
    var btnAddCurrent = document.getElementById("btn-add-current");
    var btnDelete = document.getElementById("btn-delete");
    var btnClose = document.getElementById("btn-close");

    if (btnAddManual) {
        btnAddManual.addEventListener("click", function () {
            onAddManual();
        });
    }

    if (btnAddCurrent) {
        btnAddCurrent.addEventListener("click", function () {
            triggerAction("add-current");
        });
    }

    if (btnDelete) {
        btnDelete.addEventListener("click", function () {
            onDeleteApp();
        });
    }

    if (btnClose) {
        btnClose.addEventListener("click", function () {
            triggerAction("close");
        });
    }
}

function onAddManual() {
    var nameField = document.getElementById("app-name");

    if (!nameField) return;

    var name = nameField.value.trim();

    if (name === "") {
        return;
    }

    // Set hidden inputs for C++ to read
    document.getElementById("val-app-name").value = name;
    triggerAction("add-manual");

    // Clear input after add
    nameField.value = "";
    nameField.focus();
}

function onDeleteApp() {
    var selectedItem = document.querySelector(".app-item.selected");
    if (!selectedItem) {
        return;
    }

    var name = selectedItem.getAttribute("data-name");
    if (!name) {
        return;
    }

    document.getElementById("val-app-name").value = name;
    triggerAction("delete");

    // Clear input after delete
    clearInput();
}

// Clear input field and selection - called by C++ after window picker add
function clearInput() {
    var nameField = document.getElementById("app-name");
    if (nameField) {
        nameField.value = "";
    }

    // Also clear selection
    var items = document.querySelectorAll(".app-item.selected");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }
}

function selectAppItem(element, name) {
    // Remove selected class from all items
    var items = document.querySelectorAll(".app-item");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }

    // Add selected class to clicked item
    element.classList.add("selected");

    // Fill input field
    document.getElementById("app-name").value = name;
}

function triggerAction(action) {
    var actionInput = document.getElementById("val-action");
    if (actionInput) {
        actionInput.value = action;
        // Dispatch change event for C++ to detect
        var event = new Event("change", { bubbles: true });
        actionInput.dispatchEvent(event);
    }
}

// Called by C++ to add items to the list
function addAppToList(name) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var item = document.createElement("div");
    item.className = "app-item";
    item.setAttribute("data-name", name);
    item.innerHTML = '<span class="app-item-name">' + escapeHtml(name) + '</span>';

    item.addEventListener("click", function () {
        selectAppItem(this, name);
    });

    list.appendChild(item);
}

// Called by C++ to remove a single item without full reload
function removeAppFromList(name) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var items = list.querySelectorAll('.app-item');
    for (var i = 0; i < items.length; i++) {
        if (items[i].getAttribute('data-name') === name) {
            items[i].remove();
            break;
        }
    }
}

// Called by C++ to clear the list before refreshing
function clearAppList() {
    var list = document.getElementById("app-list");
    if (list) {
        list.innerHTML = "";
    }
}

// Called by C++ after updating list to force Sciter to refresh visuals
function forceRefresh() {
    var list = document.getElementById("app-list");
    if (list) {
        // Save original display, hide, force reflow, restore
        var origDisplay = list.style.display || "";
        list.style.display = "none";
        void list.offsetHeight;  // Force reflow - void to ensure execution
        list.style.display = origDisplay || "block";

        // Also scroll to bottom to show new items
        list.scrollTop = list.scrollHeight;
    }
}

function escapeHtml(text) {
    var div = document.createElement("div");
    div.textContent = text;
    return div.innerHTML;
}
