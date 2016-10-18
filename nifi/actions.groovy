def flowFile = session.get()
def sm = context.getStateManager()
def map = new java.util.HashMap()
def state = sm.getState(org.apache.nifi.components.state.Scope.LOCAL).toMap()

if (!flowFile) return
def slurper = new groovy.json.JsonSlurper()

flowFile = session.write(flowFile,
        { inputStream, outputStream ->
            data = slurper.parse(inputStream)
            if (data.rfid_tag !=null && data.movement_z >= 10 && data.movement_x >= 4 && data.movement == "activity") {
                data.action = "pass"
                map.put(data.rfid_tag, (String.valueOf(System.currentTimeMillis()) + ":" + data.action))
            } else if (data.rfid_tag !=null && data.movement_z >= 10 && data.movement_x >= 4 && data.movement == "knock") {
                data.action = "catch"
                map.put(data.rfid_tag, (String.valueOf(System.currentTimeMillis()) + ":" + data.action))
            }

            //calulate speed
            if (data.rfid_tag !=null && (data.movement == "activity" || data.movement == "knock")) {
              def vObjJson = '{"ax0" : 0, "t": 0, "vavg": 0, "vcount": 0}'
              if (state.containsKey(data.rfid_tag + ":v")) {
                vObjJson = state.get(data.rfid_tag + ":v")
              }
              if (state.containsKey(data.rfid_tag_source + ":source:v")) {
                vObjJson = state.get(data.rfid_tag_source + ":source:v")
              }
              def vObj = slurper.parseText(vObjJson)
              if (vObj.vcount == 0 || (data.movement_time - vObj.t) <= 5000) {
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
                if (data.rfid_tag != null && data.action == "catch") {
                  //reset to start a new calc
                  map.put(data.rfid_tag + ":v", '{"ax0" : 0, "t": 0, "vavg": 0, "vcount": 0}')
                  //ensure it on the source for the catcher event
                  map.put(data.rfid_tag + ":source:v", groovy.json.JsonOutput.toJson(vObj))
                } else {
                  map.put(data.rfid_tag + ":v", groovy.json.JsonOutput.toJson(vObj))
                }

                //add data elements
                data.velocity_x = (vObj.vavg / vObj.vcount);
                data.force = data.movement_x * 350;
              } else {
                //reset
                map.put(data.rfid_tag + ":v", '{"ax0" : 0, "t": 0, "vavg": 0, "vcount": 0}')
              }
            }

            //get the state for a preivous throw.
            if (data.rfid_tag_source != null && state.containsKey(data.rfid_tag_source)) {
              //get the last action and attribute it.
              def last_action = state.get(data.rfid_tag_source)
              def action_array = last_action.split(":")
              if (System.currentTimeMillis() - Long.parseLong(action_array[0]) <= 3000) {
                data.action = action_array[1]
              }
            }

            outputStream.write(groovy.json.JsonOutput.toJson(data).getBytes("UTF-8"))
        } as StreamCallback)

sm.setState(map, org.apache.nifi.components.state.Scope.LOCAL)

// transfer
session.transfer(flowFile, REL_SUCCESS)
session.commit()
