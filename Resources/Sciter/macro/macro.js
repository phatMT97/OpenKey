// Macro Dialog JavaScript

document.ready = function () {
    initMacroDialog();
};

function initMacroDialog() {
    // Bind button clicks
    var btnAdd = document.getElementById("btn-add");
    var btnDelete = document.getElementById("btn-delete");
    var btnImport = document.getElementById("btn-import");
    var btnExport = document.getElementById("btn-export");
    var btnClose = document.getElementById("btn-close");
    var macroName = document.getElementById("macro-name");
    var macroContent = document.getElementById("macro-content");

    if (btnAdd) {
        btnAdd.addEventListener("click", function () {
            onAddMacro();
        });
    }

    if (btnDelete) {
        btnDelete.addEventListener("click", function () {
            onDeleteMacro();
        });
    }

    if (btnImport) {
        btnImport.addEventListener("click", function () {
            triggerAction("import");
        });
    }

    if (btnExport) {
        btnExport.addEventListener("click", function () {
            triggerAction("export");
        });
    }

    if (btnClose) {
        btnClose.addEventListener("click", function () {
            triggerAction("close");
        });
    }

    // Check if name field changes to switch between Add/Update
    if (macroName) {
        macroName.addEventListener("change", function () {
            updateAddButtonText();
        });
    }
}

function onAddMacro() {
    var nameField = document.getElementById("macro-name");
    var contentField = document.getElementById("macro-content");

    if (!nameField || !contentField) return;

    var name = nameField.value.trim();
    var content = contentField.value.trim();

    if (name === "" || content === "") {
        return;
    }

    // Set hidden inputs for C++ to read
    document.getElementById("val-macro-name").value = name;
    document.getElementById("val-macro-content").value = content;
    triggerAction("add");

    // Clear inputs after add
    nameField.value = "";
    contentField.value = "";
    nameField.focus();
}

function onDeleteMacro() {
    var nameField = document.getElementById("macro-name");
    if (!nameField) return;

    var name = nameField.value.trim();
    if (name === "") {
        return;
    }

    document.getElementById("val-macro-name").value = name;
    triggerAction("delete");

    // Clear inputs after delete
    nameField.value = "";
    document.getElementById("macro-content").value = "";
}

function selectMacroItem(element, name, content) {
    // Remove selected class from all items
    var items = document.querySelectorAll(".macro-item");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }

    // Add selected class to clicked item
    element.classList.add("selected");

    // Fill input fields
    document.getElementById("macro-name").value = name;
    document.getElementById("macro-content").value = content;

    // Change button text to "Sửa"
    document.getElementById("btn-add").textContent = "+ Sửa";
}

function updateAddButtonText() {
    // This will be called by C++ after checking if macro exists
    var btnAdd = document.getElementById("btn-add");
    if (btnAdd) {
        // Default to "Thêm", C++ will change to "Sửa" if macro exists
        btnAdd.textContent = "+ Thêm";
    }
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
function addMacroToList(name, content) {
    var list = document.getElementById("macro-list");
    if (!list) return;

    var item = document.createElement("div");
    item.className = "macro-item";
    item.setAttribute("data-name", name);
    item.setAttribute("data-content", content);
    item.innerHTML = '<span class="macro-item-name">' + escapeHtml(name) + '</span>' +
        '<span class="macro-item-content">' + escapeHtml(content) + '</span>';

    item.addEventListener("click", function () {
        selectMacroItem(this, name, content);
    });

    list.appendChild(item);
}

// Called by C++ to clear the list before refreshing
function clearMacroList() {
    var list = document.getElementById("macro-list");
    if (list) {
        list.innerHTML = "";
    }
}

function escapeHtml(text) {
    var div = document.createElement("div");
    div.textContent = text;
    return div.innerHTML;
}
