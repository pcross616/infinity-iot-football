import React from 'react';
import EditBar from './EditBar';

const Header = (props) => (
  <div className="top_nav">
    <div className="nav_menu">
      <nav className="dashboardHeader">
        {/* <img src={require('../assets/oracle.png')} height="40px" /> */}
        <EditBar onEdit={props.onEdit} />
      </nav>
    </div>
  </div>
);

export default Header;
