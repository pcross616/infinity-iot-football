import React from 'react';
import PropTypes from 'prop-types';

import 'odometer/odometer.min';
const debug = require('debug')('iotfootball:playerstat');

class PlayerStat extends React.Component {
  constructor() {
    super();
    this.state = {
      stats: [],
      loading: false,
    };
  }
  componentWillMount() {
    const {
      refreshInterval,
    } = this.props;

    const refreshIntervalId  = setInterval(() => this.loadData(), refreshInterval);
    this.setState(Object.assign({}, this.state, { refreshIntervalId }));
  }

  componentDidMount() {
    this.loadData();
  }

  componentWillUnmount() {
    clearInterval(this.state.refreshIntervalId);
  }

  render() {
    return (
      <div>
        {this.state.stats.map((stat, index) => (
          <div className="player-data" key={index}>
            <div className="player-header-name">{stat.player}</div>
            <hr/>
            <table className="player-data-table">
              <tbody>
                <tr className="player-data-value-row">
                  <td className="player-data-value"><span className="odometer">{stat.stats.speed ? Math.ceil(stat.stats.speed) : '0'}</span></td>
                  <td className="player-data-value"><span className="odometer">{stat.stats.pass ? stat.stats.pass : '0'}</span></td>
                  <td className="player-data-value"><span className="odometer">{stat.stats.activity ? stat.stats.activity : '0'}</span></td>
                  <td className="player-data-value"><span className="odometer">{stat.stats.knock ? stat.stats.knock : '0'}</span></td>
                </tr>
                <tr className="player-data-label-row">
                  <td className="player-data-label">MPH</td>
                  <td className="player-data-label">Catches</td>
                  <td className="player-data-label">Activity</td>
                  <td className="player-data-label">Bump</td>
                </tr>
              </tbody>
            </table>
          </div>
        ))}
      </div>
    );
  }

  loadData() {
    if (this.state.loading) {
      return; // Skip
    }

    const {
      team,
    } = this.props;

    this.setState(Object.assign({}, this.state, { loading: true }));

    fetch(`/team/${team}`)
      .then(res => res.ok ? res : Promise.reject(res.statusText))
      .then(res => res.json()).then( data => {
        if (data) {
          debug(data);
          this.setState({
            stats: data,
            loading: false,
          });
          return true;
        }
        return false;
      })
      .catch(error => console.error(`Failed to retrieve arrivals for stop ${team}:`, error));
  }
}

PlayerStat.propTypes = {
  team: PropTypes.string.isRequired,
  refreshInterval: PropTypes.number,
  limit: PropTypes.number,
};

PlayerStat.defaultProps = {
  refreshInterval: 500,
  lineIds: null,
  limit: 3,
};

export default PlayerStat;
