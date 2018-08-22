import React from 'react';
import PropTypes from 'prop-types';

import {
  ResponsiveContainer,
  Scatter,
  ScatterChart,
  XAxis,
  YAxis,
  Dot,
} from 'recharts';

class TimeSeriesChart extends React.Component {
  _drawScatter() {
    const {
      chartData,
    } = this.props;

    const areaArr = [];
    let count = 0;
    chartData.forEach(element => {
      areaArr.push(<Scatter data = {element} line = {{ stroke: this.props.color }} shape={<Dot r={0}/>} lineJointType = "monotoneX" lineType = "joint" dataKey="value" key={`scatter-${count}`}/>);
      count++;
    });
    return areaArr;
  }

  render() {
    return (
      <ResponsiveContainer width = "95%" height = {this.props.height} >
        <ScatterChart>
          <XAxis
            dataKey = "time"
            domain = {['auto', 'auto']}
            name = "Time"
            type = "number"
            hide
          />
          <YAxis dataKey = "value" name = "Value" hide/>

          {this._drawScatter()}

        </ScatterChart>
      </ResponsiveContainer>
    );
  }
}

TimeSeriesChart.propTypes = {
  chartData: PropTypes.arrayOf(
    PropTypes.arrayOf(
      PropTypes.shape({
        time: PropTypes.number,
        value: PropTypes.number,
      }))
  ).isRequired,
  color: PropTypes.string,
  height: PropTypes.number,
};

TimeSeriesChart.defaultProps = {
  color: '#eee',
  height: 200,
};

export default TimeSeriesChart;
