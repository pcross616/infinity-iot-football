import React from 'react';

export default ({title, editable, children, onRemove }) => {
  let customClass;
  switch (children.type.name.toLowerCase()) {
  case 'playerstat':
    if (children.props.team.toLowerCase().indexOf('green') > -1) {
      customClass = `widget-${children.type.name.toLowerCase()}-green`;
    } else {
      customClass = `widget-${children.type.name.toLowerCase()}-blue`;
    }
    break;
  // case 'teamheader':
  //   break;
  // case 'teamvelocity':
  //   break;
  // case 'speedometer':
  //   break;
  // case 'teampossesion':
  //   break;
  default:
    customClass = `widget-${children.type.name.toLowerCase()}`;
    break;
  }
  return (
    <div className={customClass} style={ { opacity: 1 } }>
      <div className="defaultWidgetFrame">
        {(editable && children.type.name.toLowerCase() === 'playerstat') &&
          <a className="remove" onClick={() => {onRemove();}} >Remove</a>}
        <div className="defaultWidgetFrameHeader">
          <span className="title">{title}</span>
          {editable && <a className="remove" onClick={() => {onRemove();}} >Remove</a>}
        </div>
        <div>
          {children}
        </div>
      </div>
    </div>
  );
};
