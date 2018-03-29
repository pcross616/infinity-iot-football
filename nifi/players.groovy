def flowFile = session.get()
if (!flowFile) return
def slurper = new groovy.json.JsonSlurper()

def players = ['0x04 0x96 0xa5 0x89 0xba 0x5b 0x11': 'Player 1',
             '0x04 0x96 0xa5 0x89 0xba 0x4c 0x16': 'Player 2',
             '0x04 0x96 0xa5 0x89 0xba 0x5a 0x24': 'Player 3',
             '0x04 0x96 0xa5 0x89 0xba 0x51 0x78': 'Player 4']

def teams = ['0x04 0x96 0xa5 0x89 0xba 0x5b 0x11': 'Team Green',
            '0x04 0x96 0xa5 0x89 0xba 0x4c 0x16': 'Team Green',
            '0x04 0x96 0xa5 0x89 0xba 0x5a 0x24': 'Team Blue',
            '0x04 0x96 0xa5 0x89 0xba 0x51 0x78': 'Team Blue']

//def sm = context.getStateManager()
//def map = new java.util.HashMap()
//def state = sm.getState(org.apache.nifi.components.state.Scope.LOCAL).toMap()

def props = new java.util.Properties()


def propsFile = new java.io.File('player.properties')
if (propsFile.exists()) {
  def inFile = propsFile.newDataInputStream()
  props.load(inFile)
  inFile.close()
}

flowFile = session.write(flowFile,
        { inputStream, outputStream ->
            data = slurper.parse(inputStream)
            if (data.rfid_tag != null) {
              data.player = players[data.rfid_tag]
              data.team = teams[data.rfid_tag]
              if (props.getProperty("last_tag") != null) {
                 def last_tag = props.getProperty("last_tag")
                 //def tag_array = last_tag.split(":")

                 if ( last_tag != data.rfid_tag) { //&& (System.currentTimeMillis() - Long.parseLong(tag_array[0]) <= 30000)) {
                   data.rfid_tag_source = last_tag
                 }
              }
              //this was just a rfid read not movement related.
              if (data.movement == "tag_read") {
                //map.put("last_tag", System.currentTimeMillis() + ":" + data.rfid_tag)
                props.setProperty("last_tag", data.rfid_tag)
              }
            }
            if (data.rfid_tag_source != null) {
              data.player_source = players[data.rfid_tag_source]
              data.team_source = teams[data.rfid_tag_source]
            }

            //sm.setState(map, org.apache.nifi.components.state.Scope.LOCAL)
            outputStream.write(groovy.json.JsonOutput.toJson(data).getBytes("UTF-8"))
        } as StreamCallback)

def outFile = propsFile.newDataOutputStream()
props.store(outFile, "Player State")
outFile.close()


// transfer
session.transfer(flowFile, REL_SUCCESS)
session.commit()
