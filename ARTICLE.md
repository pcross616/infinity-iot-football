Infinity Measures IoT Devices
=====
The IoT or Internet of Things is developing around us every day and the business implications are vast and far reaching. Webtrends Infinity™ was built to support the IoT, so we thought we would find a fun way to show you how this would work.

In [this video](https://www.youtube.com/watch?v=oKr_SCSRCtU&feature=youtu.be), we demonstrate a real life example of creating an IoT scenario by turning a Nerf football into an IoT Football.

While there are many IoT scenarios out there, we wanted to choose something that most of us can relate to, but perhaps haven’t thought of in an IoT context before. We have developed a prototype of the next generation football in the Webtrends lab (fully instrumented with a microprocessor, WiFi, accelerometer, and RFID reader). The purpose of this demo is to illustrate how Webtrends Infinity can measure devices in the IoT ecosystem.

For this demo, we have two teams: Team Red and Team Blue, with players on each team wearing RFID enabled gloves. We use Webtrends Infinity to process and analyze data in real time to help assess passer effectiveness and arm strength as well as track player interactions.

We can see sensor readings as the football is passed around the room in real time. This is all being done with existing Webtrends Infinity platform capabilities available today, being fed from de-facto standard IoT utilities.

## The Data

The IoT Football has some unique attributes we want to measure. Those attributes are represented as custom variables within Webtrends as follows:

**Velocity_x**: velocity of the ball in flight

 - Velocity values associated with incoming passes, higher numbers appear when a player throws harder

**Movement_x,y,z**: acceleration along each axis

 - activity = continuous motion
 - knock = a sudden bump
 - freefall = suspended motion

**Player/ Team**: derived from the RFID Tag

 - Different Players and Teams appear with counts to show the number of interactions with the football.

This is a relatively small number of variables, but you can imagine more real world scenarios having hundreds, if not thousands, of attributes we would need to collect. The Webtrends Infinity platform does not have any limitations on how much data or how many attributes can be collected so is well suited for these IoT scenarios.

## The Insights

Once the data was collected within Webtrends Infinity, we also did a deeper analysis of the data using Webtrends Infinity Analytics™. In the video, all that data you see flowing into the Webtrends Infinity data platform is available for ad hoc analysis by the Webtrends Infinity Analytics application. We used Webtrends Infinity Analytics to create measures and dimensions by assembling results of the sensor readings to provide the following insights.

 - **Team Blue** had the most completed passes and Player 2 was the best receiver from this team
 - **Player 4** from Team Blue was the Quarterback with the strongest throw (the highest recorded velocity and force).
 - **12.22 m/s2** is the average velocity of the ball passed by Team Blue’s Quarterback.
 - **4278 grams per m/s2** was the average force of passes hitting a receiver that were thrown by Player 4 (force is calculated by Infinity Analytics based off the raw data of Mass times Acceleration).

## The Implication

With this simple example, you can easily see how this IoT Football could have been a self-driving car or a drone or even your airport luggage! There will be an endless number of IoT devices out there. The challenge is going to be how to keep track of them, consolidate the mountains of sensor data and then drive actionable business insights. With Webtrends Infinity, Webtrends is ready to help businesses address these challenges today.

The power and flexibility of Webtrends Infinity goes well beyond the needs of traditional marketing analytics applications. With Infinity, you will be poised to thrive in the rapidly evolving IoT landscape.
