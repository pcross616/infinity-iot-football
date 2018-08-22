import React from 'react';
import PropTypes from 'prop-types';

import {BarChart, XAxis, Bar, ResponsiveContainer, LabelList } from 'recharts';
const debug = require('debug')('iotfootball:possesion');

class TeamPossesion extends React.Component {
  constructor() {
    super();
    this.state = {
      data: [],
      teams: [],
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

  renderCustomizedLabel(props) {
    const { x, y, width, height, value } = props;
    if (Number.isNaN(Number(value)) || value === 0) {
      return '';
    }
    if ((height / 2) > 32) {
      return (
        <g>
          <text className="possesion-label" x={x + width / 2} y={(y + height / 2)} textAnchor="middle" dominantBaseline="middle">
            {Math.round(value, 0)}
          </text>
        </g>
      );
    }
    return '';
  }

  render() {
    return (
      <ResponsiveContainer width="95%" height={300} >
        <BarChart data={this.state.data}
          margin={{ top: 20, right: 30, left: 20, bottom: 5 }}
          barCategoryGap={1}
          isAnimationActive={false} >
          <XAxis dataKey="name" />
          {this.state.teams.map((team, index) => (
            <Bar isAnimationActive={false} dataKey={team} stackId="a" key={index} fill={this.fillColor(team)} stroke={this.strokeColor(team)} >
              <LabelList dataKey={team} content={this.renderCustomizedLabel} />
            </Bar>
          ))}
        </BarChart>
      </ResponsiveContainer>
    );
  }

  fillColor(team) {
    if (team === 'Team_Green') {
      return '#3CDDB3';
    }
    return '#3A9AEA';
  }

  strokeColor(team) {
    if (team === 'Team_Green') {
      return '#3CDDB3';
    }
    return '#3A9AEA';
  }

  loadData() {
    if (this.state.loading) {
      return; // Skip
    }

    this.setState(Object.assign({}, this.state, { loading: true }));

    fetch('/team')
      .then(res => res.ok ? res : Promise.reject(res.statusText))
      .then(res => res.json()).then( data => {
        if (data) {
          debug(data);
          const arrData = [];
          const teamData = [];

          const min15 = {name: '15'};
          for (const i in data['15min']) {
            if (data['15min'].hasOwnProperty(i) && i !== 'total' && data['15min'].total !== 0) {
              const propName = i.replace(/ /g, '_');
              if (teamData.indexOf(propName) === -1) {
                teamData.push(propName);
              }
              min15[propName] = (data['15min'][i].count / data['15min'].total) * 100;
            }
          }
          arrData.push(min15);

          const min10 = {name: '10'};
          for (const i in data['10min']) {
            if (data['10min'].hasOwnProperty(i) && i !== 'total' && data['10min'].total !== 0) {
              const propName = i.replace(/ /g, '_');
              if (teamData.indexOf(propName) === -1) {
                teamData.push(propName);
              }
              min10[propName] = (data['10min'][i].count / data['10min'].total) * 100;
            }
          }
          arrData.push(min10);


          const min5 = {name: '5'};
          for (const i in data['5min']) {
            if (data['5min'].hasOwnProperty(i) && i !== 'total' && data['5min'].total !== 0) {
              const propName = i.replace(/ /g, '_');
              if (teamData.indexOf(propName) === -1) {
                teamData.push(propName);
              }
              min5[propName] = (data['5min'][i].count / data['5min'].total) * 100;
            }
          }
          arrData.push(min5);

          const min1 = {name: 'Current'};
          for (const i in data['1min']) {
            if (data['1min'].hasOwnProperty(i) && i !== 'total' && data['1min'].total !== 0) {
              const propName = i.replace(/ /g, '_');
              if (teamData.indexOf(propName) === -1) {
                teamData.push(propName);
              }
              min1[propName] = (data['1min'][i].count / data['1min'].total) * 100;
            }
          }
          arrData.push(min1);

          this.setState({
            data: arrData,
            teams: teamData,
            loading: false,
          });
          return true;
        }
        return false;
      })
      .catch(error => console.error('Failed to retrieve possesion data:', error));
  }
}

TeamPossesion.propTypes = {
  refreshInterval: PropTypes.number,
};

TeamPossesion.defaultProps = {
  refreshInterval: 1000,
};

export default TeamPossesion;
