// Time page
function setTime(hour, minute) {
    var date = new Date();
    date.setHours(hour);
    date.setMinutes(minute);
    date.setSeconds(0);
    date.setMilliseconds(0);
    date = new Date(date.getTime() - date.getTimezoneOffset()*60000);
    wsCommand("time set " + date.toISOString());
}

$$(document).on('page:init', '.page[data-name="time"]', function (e) {
    wsCommand("time get", function(response) {
        if (!response.time) { return; }
        var time = new Date(new Date(response.time.slice(0,-1)).getTime() + new Date().getTimezoneOffset() * 60000);
        $$("#hour").val(time.getHours());
        $$("#minute").val(time.getMinutes());
    });
});