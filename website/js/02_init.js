// Initialize your app
var app = new Framework7({
    root: '#app',
    name: 'LED Clock',
    init: false,
    theme: 'ios',
    routes: [
        {
            path: '/:page*',
            url: '/{{page}}'
        }
    ]
});

// Export selectors engine
var $$ = Dom7;

// Add view
var mainView = app.views.create('.view-main', {
    //iosDynamicNavbar: false,
    pushState: true
});