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
            
            //calulate speed
            if (((data.movement == "activity" || data.movement == "knock") || data.action == "catch")) {
              def vObjJson = '{"ax0" : 0, "ay0" : 0, "az0" : 0, "t": 0, "j": 0, "vavg": 0, "vcount": 0, "rfid_tag": ""}'
              if (props.getProperty(data.dcsvid + ":v") != null) {
                vObjJson = props.getProperty(data.dcsvid + ":v")
              }

              def vObj = slurper.parseText(vObjJson)
              // too long between window reset or new person
              if ((data.m_time - vObj.t) >= 5000 || data.rfid_tag != vObj.rfid_tag) {
                vObjJson = '{"ax0" : 0, "ay0" : 0, "az0" : 0, "t": 0, "j": 0, "vavg": 0, "vcount": 0, "rfid_tag": ""}'
                data.p_time = 20 //give 1/2 sec by default
                //reset
                props.remove(data.dcsvid + ":v")
                vObj = slurper.parseText(vObjJson)
              }

              if (vObj.vcount == 0 || vObj.j == null) {
                vObj.j = data.m_time;
              }
              //seed with the current speed if its the first time
              def vx = data.m_x
              def vy = data.m_y
              def vz = data.m_z
              def pt = 20; //give them 20ms by default
              def v = 0
              def btop = 0
              def bbot = 0

              if (vObj.vcount != 0) {
                //v = ax0 + ax1t
                vx = vObj.ax0 + (data.m_x * (data.m_time - vObj.t))  
                vy = vObj.ay0 + (data.m_y * (data.m_time - vObj.t))  
                vz = vObj.az0 + (data.m_z * (data.m_time - vObj.t))
                //v = java.lang.Math.sqrt(java.lang.Math.pow(vx,2) + java.lang.Math.pow(vy,2) + java.lang.Math.pow(vz,2))
                btop = btop + ((data.m_time - vObj.t) * v)
                bbot = bbot + java.lang.Math.pow((data.m_time - vObj.t),2)
                pt = data.m_time - vObj.j
              }

              v = java.lang.Math.sqrt(java.lang.Math.pow(vx,2) + java.lang.Math.pow(vy,2) + java.lang.Math.pow(vz,2))
              
              vObj.t = data.m_time
              vObj.ax0 = data.m_x
              vObj.ay0 = data.m_y
              vObj.az0 = data.m_z
              vObj.vavg = vObj.vavg + vx
              vObj.vcount++;

              //update map
              if (data.movement == "knock") {
                //reset to start a new calc
                props.remove(data.dcsvid + ":v")
              } else {
                props.setProperty(data.dcsvid + ":v", groovy.json.JsonOutput.toJson(vObj))
              }

              //add data elements
              data.velocity_x = v //java.lang.Math.abs((vObj.vavg / vObj.vcount));
              data.velocity_avg = java.lang.Math.abs((vObj.vavg / vObj.vcount));
              data.force = data.m_x * 350;
              data.p_time = pt 
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
