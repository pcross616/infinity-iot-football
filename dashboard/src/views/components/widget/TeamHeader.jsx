import React from 'react';
import SVG from 'react-svg-inline';
import PropTypes from 'prop-types';

import Helmet from '../../../../public/assets/football-helmet.svg';

class TeamHeader extends React.Component {
  render() {
    const {
      direction,
    } = this.props;

    return (
      <div className="helmet-icon"><SVG svg={Helmet} className={'team-header-' + direction + '-icon'}/></div>
    );
  }
}

TeamHeader.propTypes = {
  direction: PropTypes.string,
};

TeamHeader.defaultProps = {
  direction: 'right',
};

export default TeamHeader;
