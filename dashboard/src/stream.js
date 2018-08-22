const fs = require('fs');
const debug = require('debug')('iotfootball:stream');
const Websocket = require('ws');
const config = require('./config.js');

module.exports = (error, message) => {
  let ws = null;
  function reconnect() {
    const wsHeaders = {'Authorization': 'Basic ' + new Buffer(config.username + ':' + config.password, 'utf8').toString('base64')};

    if (config.root_cert) {
      fs.readFile(config.root_cert, (err, data) => {
        if (err) throw err;

        ws = new Websocket(config.streamUrl, null,
          {
            headers: wsHeaders,
            ca: data,
          });
      });
    } else {
      ws = new Websocket(config.streamUrl, null,
        {
          headers: wsHeaders,
        });
    }

    ws.on('open', () => {
      debug('websocket connected..');
      try {
        ws.send(JSON.stringify({
          'command': 'stream',
          'account_guid': config.account_guid,
          'stream_type': 'session',
          'query': config.query,
          'api_version': '2.0',
          'schema_version': '2.0',
        }));
      } catch (e) {
        // fix
        console.error(e);
      }
    });

    ws.on('message', message);

    ws.on('close', () => {
      debug('websocket disconnected.. will reconnect in 500ms');
      setTimeout(reconnect, 500);
    });

    ws.on('error', () => {
      console.error('websocket error, now disconnected.');
    });
  }

  process.on('SIGINT', () => {
    console.log('Closing websocket on ctrl-c');
    ws.close();
    process.exit();
  });


  process.once('SIGUSR2', () => {
    console.log('Closing websocket on source change restart');
    ws.close();
    process.kill(process.pid, 'SIGUSR2');
  });

  reconnect();
};
