const debug = require('debug')('iotfootball:process');

module.exports = (hit, dbs) => {
  const data = hit.event[0].data;
  const now = new Date().getTime();

  if (!data) {
    return;
  }

  data._ts = now;

  // store events as numbers where needed.
  for (const prop in data) {
    const num = Number(data[prop]);
    if (!Number.isNaN(num)) {
      data[prop] = num;
    }
  }

  if (data.p_time && data.p_time === 20) {
    data.p_time = data.p_time / Math.random();
  }

  dbs.events.insert(data);

  // do we have a player
  if (data.rfid_tag) {
    dbs.playerstats.update(
      {rfid_tag: data.rfid_tag}, {
        $set: {
          'rfid_tag': data.rfid_tag,
          'player': data.player,
          'team': data.team,
          '_ts': data._ts,
        },
      },
      {upsert: true}, () => {});
    const obj = {_ts: data._ts};
    if (data.action) {
      dbs.events.count({$and: [{rfid_tag: data.rfid_tag}, {action: data.action}]}, (err, count) => {
        obj['stats.' + data.action] = count;
      });
    }
    if (data.movement) {
      dbs.events.count({$and: [{rfid_tag: data.rfid_tag}, {movement: data.movement}]}, (err, count) => {
        obj['stats.' + data.movement] = count;
      });
    }
    if (data.velocity_x) {
      dbs.events.find({$and: [{rfid_tag: data.rfid_tag}, {velocity_x: {$exists: true}}, {velocity_x: { $lt: 100 }}]}, (err, docs) => {
        let totals = 0;
        docs.forEach((value) => {
          totals += value.velocity_x;
        });
        obj['stats.speed'] = (totals / docs.length) * 0.4469; // translate to mph
      });
    }
    dbs.playerstats.update(
      {rfid_tag: data.rfid_tag}, {
        $set: obj,
      },
      {upsert: true}, () => {});
  }
  debug(data);
};
