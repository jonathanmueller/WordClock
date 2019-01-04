function showUpdateStatus(success) {
    if (success) { app.dialog.alert("Update erfolgreich eingespielt!", "Erfolg");  }
    else { app.dialog.alert("Fehler beim Hochladen des Updates.", "Fehler"); }
}

// Firmwareupdate page
$$(document).on('page:init', '.page[data-name="firmwareupdate"]', function (e) {
    var forms = e.detail.$el.find("form");
    forms.on('formajax:success', function (formEl, data, xhr) { try { showUpdateStatus(JSON.parse(xhr.responseText).success); } catch (e) { showUpdateStatus(false); } });
    forms.on('formajax:error', function (formEl, data, xhr) { showUpdateStatus(false); });
    forms.on('formajax:beforesend', function (formEl, data, xhr) { app.dialog.progress("Update wird hochgeladen..."); });
    forms.on('formajax:complete', function (formEl, data, xhr) { app.dialog.close(); });
});