// TODO
// - refactor out the long functions

const express = require('express');
const logger = require('morgan');
const cookieParser = require('cookie-parser');
const bodyParser = require('body-parser');
const streamProcess = require('./stream.js');
const processData = require('./process.js');
const DataStore = require('nedb');
const path = require('path');
const debug = require('debug')('iotfootball:app');


module.exports = (app) => {
  // hold all the datastore objects
  const dbs = {};

  dbs.events = new DataStore();

  dbs.events.ensureIndex(
    {fieldName: '_ts', expireAfterSeconds: 900}, () => {});

  dbs.playerstats = new DataStore();

  dbs.playerstats.ensureIndex(
    {fieldName: '_ts', expireAfterSeconds: 900}, () => {});

  function msgAction(streamData) {
    if (streamData) {
      try {
        const hit = JSON.parse(streamData);
        if (hit.event) {
          processData(hit, dbs);
        }
      } catch (e) {
        console.error(e);
        console.error(streamData);
      }
    }
  }

  // Connect stream to processor
  streamProcess(null, msgAction);


  process.on('uncaughtException', (err) => {
    console.error(
      (new Date).toUTCString() + ' uncaughtException:', err.message);
    console.error(err.stack);
    process.exit(1);
  });

  // uncomment after placing your favicon in /public

  app.use(logger('dev'));
  app.use(bodyParser.json());
  app.use(bodyParser.urlencoded({extended: false}));
  app.use(cookieParser());

  const distPath = path.resolve('./dist');
  const publicPath = path.resolve('./public');
  console.info(distPath);
  const indexFileName = 'index.html';
  app.use(express.static(distPath));
  app.get('/', (req, res) => res.sendFile(indexFileName, {root: distPath}));
  app.use('/public', express.static(publicPath));

  // REST endpoints
  app.use('/team/:id/events/:limit/:type', (req, res) => {
    dbs.events.find({$and: [{team: req.params.id}, {velocity_x: {$exists: true}}, {velocity_x: { $lt: 100 }}, {movement: req.params.type}]})
      .sort({_ts: -1, m_time: -1})
      .limit(req.params.limit)
      .exec((err, docs) => {
        // create the main array to store data
        const results = [];
        docs.forEach((event) => {
          results.push([{time: 0, value: 0}, {time: (event.p_time / 2), value: event.velocity_x}, {time: (event.p_time === 20 ? Math.sqrt(event.p_time) + event.p_time : event.p_time), value: 0}]);
        });
        res.send(results);
      });
  });

  app.use('/team/:id/events/:limit', (req, res) => {
    dbs.events.find({team: req.params.id})
      .sort({_ts: -1, m_time: -1})
      .limit(req.params.limit)
      .exec((err, result) => {
        res.send(result);
      });
  });

  app.use('/team/:id', (req, res) => {
    dbs.playerstats.find({team: req.params.id}, (err, result) => {
      res.send(result);
    });
  });

  app.use('/team', (req, res) => {
    dbs.events
      .find({
        $and: [
          {$or: [{'movement': 'activity'}, {'movement': 'knock'}]},
          {rfid_tag: {$exists: true}}, {team: {$exists: true}},
          {p_time: {$exists: true}},
        ],
      })
      .sort({team: -1, _ts: -1, m_time: -1})
      .exec((err, result) => {
        const obj = {};
        obj['1min'] = {total: 0};
        obj['5min'] = {total: 0};
        obj['10min'] = {total: 0};
        obj['15min'] = {total: 0};
        const now = new Date().getTime();

        result.forEach((value) => {
          const diff = now - value._ts;
          if (!obj['1min'][value.team]) {
            obj['1min'][value.team] = {count: 0};
          }
          if (!obj['5min'][value.team]) {
            obj['5min'][value.team] = {count: 0};
          }
          if (!obj['10min'][value.team]) {
            obj['10min'][value.team] = {count: 0};
          }
          if (!obj['15min'][value.team]) {
            obj['15min'][value.team] = {count: 0};
          }

          if (value.p_time) {
            if (diff <= 60000) {
              obj['1min'][value.team].count += value.p_time;
              obj['1min'].total += value.p_time;
            }
            if (diff > 60000 && diff <= 300000) {
              obj['5min'][value.team].count += value.p_time;
              obj['5min'].total += value.p_time;
            }
            if (diff > 300000 && diff <= 600000) {
              obj['10min'][value.team].count += value.p_time;
              obj['10min'].total += value.p_time;
            }
            if (diff > 600000) {
              obj['15min'][value.team] += value.p_time;
              obj['15min'].total += value.p_time;
            }
          }
        });

        res.send(obj);
      });
  });

  app.use('/event/last/speed', (req, res) => {
    dbs.events
      .find(
        {$and: [{movement: 'activity'}, {velocity_x: {$exists: true}}]}
      )
      .sort({_ts: -1, m_time: -1})
      .limit(1)
      .exec((err, result) => {
        res.send(result);
      });
  });

  app.use('/event/last', (req, res) => {
    dbs.events
      .find({
        $or: [
          {'action': 'pass'}, {'movement': 'activity'},
          {'movement': 'knock'},
        ],
      })
      .sort({_ts: -1, m_time: -1})
      .limit(1)
      .exec((err, result) => {
        res.send(result);
      });
  });

  app.use('/event', (req, res) => {
    dbs.events.find({}, (err, result) => {
      res.send(result);
    });
  });

  // catch 404 and forward to error handler
  app.use((req, res, next) => {
    const err = new Error('Not Found');
    err.status = 404;
    next(err);
  });

  // error handlers

  // development error handler
  // will print stacktrace
  if (app.get('env') === 'development') {
    app.use((err, req, res) => {
      res.status(err.status || 500);
      debug(err);

      res.render('Error', {
        message: err.message,
        error: err,
      });
    });
  }

  // production error handler
  // no stacktraces leaked to user
  app.use((err, req, res) => {
    res.status(err.status || 500);
    res.render('Error', {
      message: err.message,
      error: {},
    });
  });

  console.log('IoT Football Dashboard Data Processor UP: ' + new Date());
};
