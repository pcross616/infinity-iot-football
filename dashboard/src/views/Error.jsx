const React = require('react');

class ErrorMessage extends React.Component {
  render() {
    return <div>Message: {this.props.message}</div>;
  }
}

module.exports = ErrorMessage;
