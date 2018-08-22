
import React from 'react';
import PropTypes from 'prop-types';
import { PieChart, Pie, Cell, Label, ResponsiveContainer} from 'recharts';

// const data = [{ value: 30 }, { value: 20 }];
const speed = [{ value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 }, { value: 51 },
  { value: 51 }];
const COLORS = ['#2FB895', '#737E8D'];

const RADIAN = Math.PI / 180;
class Speedometer extends React.Component {
  constructor() {
    super();
    this.state = {
      data: [{value: 0}, {value: speed.length}],
      event: null,
      leftprev: 0,
      rightprev: 0,
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

  renderCustomizedLabel({ cx, cy, midAngle, innerRadius, outerRadius, percent, index }) { /* eslint-disable-line no-unused-vars */
    const radius = innerRadius + (outerRadius - innerRadius) * -15;
    const x = cx + radius * Math.cos(-midAngle * RADIAN);
    const y = cy + radius * Math.sin(-midAngle * RADIAN);
    if ((index % 10) === 0) {
      return (
        <text className="speed-inc-label" x={x} y={y} fill="#737E8D" textAnchor={x > cx ? 'start' : 'end'} dominantBaseline="central">{index}</text>
      );
    }
    return '';
  }

  renderCenterLabel(props) {
    const { viewBox, offset, position, value } = props; /* eslint-disable-line no-unused-vars */
    const { cx, cy, innerRadius, outerRadius, startAngle, endAngle } = viewBox; /* eslint-disable-line no-unused-vars */
    const obj = JSON.parse(value);
    const teamcss = obj && obj.team === 'Team Green' ? 'speed-player-right-label' : 'speed-player-left-label';
    return (
      <text x={cx} y={cy} textAnchor="middle">
        <tspan x={cx} dy={-10} className="speed-last-label">{obj ? Math.ceil(obj.velocity_x * 0.4469) : '-'}</tspan>
        <tspan x={cx} dy={70} className="speed-mph-label">mph</tspan>
        <tspan x={cx} dy={80} className={teamcss}>{obj ? obj.player : ''}</tspan>
      </text>
    );
  }

  render() {
    return (
      <ResponsiveContainer width="100%" height={425} >
        <PieChart>
          <Pie
            data={[{value: 1}]}
            dataKey="value"
            cx="25%"
            cy="20%"
            startAngle={260}
            endAngle={20}
            innerRadius={70}
            outerRadius={80}
            fill="#737E8D"
            isAnimationActive={false}
            paddingAngle={0}>
            <Cell fill="#737E8D" stroke={0} />
            <Label className="speed-prev-left-label" value={this.state.leftprev} offset={0} position="center" />
          </Pie>
          <Pie
            data={[{value: 1}]}
            dataKey="value"
            cx="75%"
            cy="20%"
            startAngle={-80}
            endAngle={160}
            innerRadius={70}
            outerRadius={80}
            fill="#737E8D"
            isAnimationActive={false}
            paddingAngle={0}>
            <Cell fill="#737E8D" stroke={0}/>
            <Label className="speed-prev-right-label" value={this.state.rightprev} offset={0} position="center" />
          </Pie>
          <Pie
            data={this.state.data}
            dataKey="value"
            startAngle={210}
            endAngle={-30}
            innerRadius={165}
            outerRadius={175}
            fill="#8884d8"
            isAnimationActive={false}
            paddingAngle={0}>
            {
              this.state.data.map((entry, index) => <Cell key={`cell-${index}`} fill={COLORS[index % COLORS.length]} stroke={0}/>)
            }
            <Label content={this.renderCenterLabel} value={JSON.stringify(this.state.event)} offset={0} position="center" />
          </Pie>
          <Pie
            data={speed}
            dataKey="value"
            startAngle={210}
            endAngle={-30}
            innerRadius={182}
            outerRadius={185}

            label={this.renderCustomizedLabel}
            fill="#8884d8"
            paddingAngle={0}
            isAnimationActive={false}
          >
            {
              speed.map((entry, index) => <Cell key={`speed-${index}`} fill={(index % 10 === 0 ? '#737E8D' : '#404a59')} stroke={0} />)
            }
          </Pie>

        </PieChart>
      </ResponsiveContainer>
    );
  }

  loadData() {
    if (this.state.loading) {
      return; // Skip
    }


    this.setState(Object.assign({}, this.state, { loading: true }));

    fetch('/event/last/speed')
      .then(res => res.ok ? res : Promise.reject(res.statusText))
      .then(res => res.json()).then( event => {
        if (event[0]) {
          if (event[0].velocity_x) {
            if (event[0].velocity_x > 100) {
              event[0].velocity_x = 100;
            }
            const es = Math.ceil(event[0].velocity_x * 0.4469);
            let prevleft = this.state.leftprev;
            let prevright = this.state.rightprev;

            // such a hack...
            COLORS[0] = event[0].team === 'Team Green' ? '#2FB895' : '#3A9AEA';
            if (this.state.event) {
              if (this.state.event.team === 'Team Green') {
                prevright = Math.ceil(this.state.event.velocity_x * 0.4469);
              } else if (this.state.event.team === 'Team Blue') {
                prevleft = Math.ceil(this.state.event.velocity_x * 0.4469);
              }
            }

            this.setState({
              data: [{value: es}, {value: (speed.length - es)}],
              event: event[0],
              leftprev: prevleft,
              rightprev: prevright,
              loading: false,
            });
          } else {
            this.setState({
              data: this.state.data,
              event: this.state.event,
              leftprev: this.state.leftprev,
              rightprev: this.state.rightprev,
              loading: false,
            });
          }
          return true;
        }
        this.setState({
          data: [{value: 0}, {value: speed.length}],
          event: null,
          leftprev: 0,
          rightprev: 0,
          loading: false,
        });
        return false;
      })
      .catch(error => console.error('Failed to retrieve last event data:', error));
  }
}

Speedometer.propTypes = {
  refreshInterval: PropTypes.number,
};

Speedometer.defaultProps = {
  refreshInterval: 500,
};
export default Speedometer;
