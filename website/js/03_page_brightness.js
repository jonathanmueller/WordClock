$$(document).on('page:init', '.page[data-name="brightness"]', function (e) {
    wsCommand("settings get brightness", function (response) {
        if (!response.settings || !response.settings.brightness) { return; }
        app.range.setValue('#brightnessSlider', response.settings.brightness);
    });
});