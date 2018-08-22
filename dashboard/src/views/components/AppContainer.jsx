import React from 'react';
import Dashboard, { addWidget } from 'react-dazzle';

// Default styles.
import 'react-dazzle/lib/style/style.css';

// App Components
import Header from './Header';
import Container from './Container';
import CustomFrame from './CustomFrame';

// Widgets
import PlayerStat from './widget/PlayerStat';
import Speedometer from './widget/Speedometer';

import TeamPossesion from './widget/TeamPossesion';
import TeamVelocity from './widget/TeamVelocity';
import TeamHeader from './widget/TeamHeader';

import AddWidgetDialog from './AddWidgetDialog';
// import CustomAddWidgetButton from './CustomAddWidgetButton';

import 'bootstrap/dist/css/bootstrap.css';
import 'react-dazzle/lib/style/style.css';
import '../../../public/css/custom.css';

class AppContainer extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      layout: {
        rows: [{
          columns: [{
            className: 'col-lg-3',
            widgets: [{ key: 'TeamBlueHeader' }, { key: 'TeamBlueVelocity' }, { key: 'TeamBlueStat' }],
          }, {
            className: 'col-lg-6',
            widgets: [{key: 'Speedometer' }, {key: 'TeamPossesion' }],
          }, {
            className: 'col-lg-3',
            widgets: [{ key: 'TeamGreenHeader' }, { key: 'TeamGreenVelocity' }, { key: 'TeamGreenStat' }  ],
          }],
        }]},
      widgets: {
        TeamBlueHeader: {
          type: TeamHeader,
          title: 'Team Blue',
          props: {
            team: 'Team Blue',
            direction: 'right',
          },
        },
        TeamGreenHeader: {
          type: TeamHeader,
          title: 'Team Green',
          props: {
            team: 'Team Green',
            direction: 'left',
          },
        },
        TeamGreenVelocity: {
          type: TeamVelocity,
          title: 'Velocity',
          props: {
            team: 'Team Green',
          },
        },
        TeamBlueVelocity: {
          type: TeamVelocity,
          title: 'Velocity',
          props: {
            team: 'Team Blue',
          },
        },
        TeamBlueStat: {
          type: PlayerStat,
          title: 'Team Blue',
          props: {
            team: 'Team Blue',
          },
        },
        TeamGreenStat: {
          type: PlayerStat,
          title: 'Team Green',
          props: {
            team: 'Team Green',
          },
        },
        Speedometer: {
          type: Speedometer,
          title: 'Speed',
        },
        TeamPossesion: {
          type: TeamPossesion,
          title: 'Possesion',
        },
      },
      editMode: false,
      isModalOpen: false,
      addWidgetOptions: null,
    };
  }

  onRemove = (layout) => {
    this.setState({
      layout,
    });
  }

  onAdd = (layout, rowIndex, columnIndex) => {
    this.setState({
      isModalOpen: true,
      addWidgetOptions: {
        layout,
        rowIndex,
        columnIndex,
      },
    });
  }

  onMove = (layout) => {
    this.setState({
      layout,
    });
  }

  onRequestClose = () => {
    this.setState({
      isModalOpen: false,
    });
  }

  render() {
    /* eslint max-len: "off" */
    return (
      <Container>
        <Header onEdit={this.toggleEdit} />
        <Dashboard
          onRemove={this.onRemove}
          layout={this.state.layout}
          widgets={this.state.widgets}
          editable={this.state.editMode}
          addWidgetComponentText="Add"
          onAdd={this.onAdd}
          onMove={this.onMove}
          frameComponent={CustomFrame}
        />
        <AddWidgetDialog widgets={this.state.widgets} isModalOpen={this.state.isModalOpen} onRequestClose={this.onRequestClose} onWidgetSelect={this.widgetSelected} />
      </Container>
    );
  }

  toggleEdit = () => {
    this.setState({
      editMode: !this.state.editMode,
    });
  };

  widgetSelected = (widgetName) => {
    const { layout, rowIndex, columnIndex } = this.state.addWidgetOptions;
    this.setState({
      layout: addWidget(layout, rowIndex, columnIndex, widgetName),
    });
    this.onRequestClose();
  }
}

export default AppContainer;
