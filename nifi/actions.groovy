def flowFile = session.get()
//def sm = context.getStateManager()
//def map = new java.util.HashMap()
//def state = sm.getState(org.apache.nifi.components.state.Scope.LOCAL).toMap()

def props = new java.util.Properties()


def propsFile = new java.io.File('action.properties')
if (propsFile.exists()) {
  def inFile = propsFile.newDataInputStream()
  props.load(inFile)
  inFile.close()
}



if (!flowFile) return
def slurper = new groovy.json.JsonSlurper()

flowFile = session.write(flowFile,
        { inputStream, outputStream ->
            data = slurper.parse(inputStream)
            // if (data.rfid_tag !=null && data.movement_z >= 5 && data.movement_x >= 4 && data.movement == "activity") {
            //     data.action = "pass"
            //     //map.put(data.rfid_tag, (String.valueOf(System.currentTimeMillis()) + ":" + data.action))
            // } else
            if (data.rfid_tag !=null && data.movement_z >= 5 && data.movement_x >= 4 && data.movement == "knock") {
                data.action = "pass"
                props.setProperty(data.rfid_tag, "catch")
            }

            //get the state for a preivous throw, do they have catch.
            if (data.rfid_tag_source != null && props.getProperty(data.rfid_tag_source) != null) {
              //get the last action and attribute it.
              //def last_action = state.get(data.rfid_tag_source)
              //def action_array = last_action.split(":")
              // if (System.currentTimeMillis() - Long.parseLong(action_array[0]) <= 30000) {
              //   data.action = action_array[1]
              // }
              data.action = props.getProperty(data.rfid_tag_source)
              props.remove(data.rfid_tag_source)
            }

            //calulate speed
            if (data.rfid_tag !=null && ((data.movement == "activity" || data.movement == "knock") || data.action == "catch")) {
              def vObjJson = '{"ax0" : 0, "t": 0, "vavg": 0, "vcount": 0}'
              if (props.getProperty(data.rfid_tag + ":v") != null) {
                vObjJson = props.getProperty(data.rfid_tag + ":v")
              }
              if (props.getProperty(data.rfid_tag_source + ":source:v") != null) {
                vObjJson = props.getProperty(data.rfid_tag_source + ":source:v")
                //clear it
                props.remove(data.rfid_tag_source + ":source:v")
              }
              def vObj = slurper.parseText(vObjJson)
              if (data.action != "catch" && (vObj.vcount == 0 || (data.movement_time - vObj.t) <= 5000)) {
                //seed with the current speed if its the first time
                def v = data.movement_x
                if (vObj.vcount != 0) {
                  //v = ax0 + ax1t
                  v = vObj.ax0 + (data.movement_x * (data.movement_time - vObj.t))
                }
                vObj.ax0 = data.movement_x
                vObj.t = data.movement_time
                vObj.vavg = vObj.vavg + v
                vObj.vcount++;

                //update map
                if (data.rfid_tag != null && data.action == "pass") {
                  //reset to start a new calc
                  props.remove(data.rfid_tag + ":v")
                  //ensure it on the source for the catcher event
                  props.setProperty(data.rfid_tag + ":source:v", groovy.json.JsonOutput.toJson(vObj))
                } else if (data.action != "catch") {
                  props.getProperty(data.rfid_tag + ":v", groovy.json.JsonOutput.toJson(vObj))
                }

                //add data elements
                data.velocity_x = (vObj.vavg / vObj.vcount);
                data.force = data.movement_x * 350;
              } else if (data.action == "catch") {
                //add data elements
                data.velocity_x = (vObj.vavg / vObj.vcount);
                data.force = vObj.ax0 * 350;
              } else {
                //reset
                props.remove(data.rfid_tag + ":v")
              }
            }
            //sm.setState(map, org.apache.nifi.components.state.Scope.LOCAL)
            outputStream.write(groovy.json.JsonOutput.toJson(data).getBytes("UTF-8"))
        } as StreamCallback)

def outFile = propsFile.newDataOutputStream()
props.store(outFile, "Action State")
outFile.close()

// transfer
session.transfer(flowFile, REL_SUCCESS)
session.commit()
