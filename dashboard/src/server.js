/**
 * Module dependencies.
 */

const nodeApp = require('./app');
const express = require('express');
const helmet = require('helmet');

const isDeveloping = process.env.NODE_ENV !== 'production';
const port = isDeveloping ? 3000 : process.env.PORT;
let started = false;

/**
 * Get port from environment and store in Express.
 */
const app = express();

app.set('port', port);

// Enable various security helpers.
app.use(helmet());

// start application
nodeApp(app);

/**
 * Listen on provided port, on all network interfaces.
 */
function startEndpoint() {
  app.listen(port, '0.0.0.0', function onStart(err) {
    started = true;
    if (err) {
      console.log(err);
    }
    console.info('==> Listening on port %s. Open up http://0.0.0.0:%s/ in your browser.', port, port);
  });
}

exports.start = () => {
  if (started) {
    console.info('==> Server already running, reloading.');
    return;
  }
  startEndpoint();
};

if (!module.parent) {
  startEndpoint();
}
