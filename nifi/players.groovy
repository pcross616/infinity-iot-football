def flowFile = session.get()
if (!flowFile) return
def slurper = new groovy.json.JsonSlurper()

def players = ['0x04 0x96 0xa5 0x89 0xba 0x5b 0x11': 'Player 1',
             '0x04 0x96 0xa5 0x89 0xba 0x4c 0x16': 'Player 2',
             '0x04 0x96 0xa5 0x89 0xba 0x5a 0x24': 'Player 3',
             '0x04 0x96 0xa5 0x89 0xba 0x51 0x78': 'Player 4']

def teams = ['0x04 0x96 0xa5 0x89 0xba 0x5b 0x11': 'Team Red',
            '0x04 0x96 0xa5 0x89 0xba 0x4c 0x16': 'Team Red',
            '0x04 0x96 0xa5 0x89 0xba 0x5a 0x24': 'Team Blue',
            '0x04 0x96 0xa5 0x89 0xba 0x51 0x78': 'Team Blue']

def sm = context.getStateManager()
def map = new java.util.HashMap()
def state = sm.getState(org.apache.nifi.components.state.Scope.LOCAL).toMap()

flowFile = session.write(flowFile,
        { inputStream, outputStream ->
            data = slurper.parse(inputStream)
            if (data.rfid_tag != null) {
              data.player = players[data.rfid_tag]
              data.team = teams[data.rfid_tag]
              if (state.containsKey("last_tag")) {
                 def last_tag = state.get("last_tag")
                 def tag_array = last_tag.split(":")

                 if (!(tag_array[1].equals(data.rfid_tag)) && (System.currentTimeMillis() - Long.parseLong(tag_array[0]) <= 30000)) {
                   data.rfid_tag_source = tag_array[1]
                }
              }
              map.put("last_tag", System.currentTimeMillis() + ":" + data.rfid_tag)

            }
            if (data.rfid_tag_source != null) {
              data.player_source = players[data.rfid_tag_source]
              data.team_source = teams[data.rfid_tag_source]
            }

            outputStream.write(groovy.json.JsonOutput.toJson(data).getBytes("UTF-8"))
        } as StreamCallback)

sm.setState(map, org.apache.nifi.components.state.Scope.LOCAL)
// transfer
session.transfer(flowFile, REL_SUCCESS)
session.commit()
