import React from 'react';
import PropTypes from 'prop-types';

import {Line} from 'recharts';
import TimeSeriesChart from './TimeSeriesChart';

class TeamVelocity extends React.Component {
  _drawLines() {
    const data = this.state.stats || [];
    const dataSet = data[0];
    const areaArr = [];
    let count = 0;
    for (const i in dataSet) {
      if (dataSet.hasOwnProperty(i) && i !== 'name') {
        areaArr.push(<Line type="monotone" dataKey={i} stroke="#8884d8" key={`line-chart-${count}`}/>);
        count++;
      }
    }
    return areaArr;
  }

  constructor() {
    super();
    this.state = {
      data: [],
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
        {/* <TimeSeriesChart height={150} chartData={[[{time: 0, value: 0}, {time: 3, value: 23}, {time: 5, value: 0}], [{time: 0, value: 0}, {time: 4, value: 15}, {time: 6, value: 0}]]}/> */}
        <TimeSeriesChart height={150} chartData={this.state.data}/>
      </div>
    );
  }

  loadData() {
    if (this.state.loading) {
      return; // Skip
    }

    const {
      team,
      limit,
    } = this.props;

    this.setState(Object.assign({}, this.state, { loading: true }));

    fetch(`/team/${team}/events/${limit}/activity`)
      .then(res => res.ok ? res : Promise.reject(res.statusText))
      .then(res => res.json()).then( data => {
        if (data) {
          this.setState({
            data: data,
            loading: false,
          });
          return true;
        }
        return false;
      })
      .catch(error => console.error(`Failed to retrieve data for team ${team}:`, error));
  }
}

TeamVelocity.propTypes = {
  team: PropTypes.string.isRequired,
  refreshInterval: PropTypes.number,
  limit: PropTypes.number,
};

TeamVelocity.defaultProps = {
  refreshInterval: 1500,
  limit: 5,
};

export default TeamVelocity;
