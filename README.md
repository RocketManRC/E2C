# E2C
Pebble App for the Eco-Endurance Challenge (E2C).

This Pebble Watch App was created for use during the 2016 Eco-Endurance Challenge which is a fund raiser for Ground Search and Rescue in Halifax, Nova Scotia, Canada (see http://ecoendurancechallenge.ca/).

This app allows you to navigate to control points by entering their UTM coordinates. It has the following features:

- On launch, displays the last 5 digits of the UTM Easting and Northing of your current position (so you can look up on the map)
- The enter button brings up two screens that allow you to enter the destination Easting and Northing (last 5 digits)
- The main screen will switch to display the range and bearing to the destination as well as the GPS course/heading.
- Bearings are in magnetic so can be used with a compass directly.
- From the main screen the up button will save the current destination and the down button will load it. 

The sample Weather App was the starting point for this one as it uses the pebblekit.js location services.

The calculations for range and bearing and Lat/Lon to and from UTM are done on the phone using Pebblekit.js. The code for this is all collected from various public domain websites and references are in the javascript file.

NOTE: this app would have to be modified to use it in another area as it has the magnetic deviation and first digit of the Easting and first two digits of the Northing hard coded (should be fairly easy to find in the javascript file).

DISCLAIMER: This app contains open source software from a variety of places and should be considered experimental. If you use this or a derivative for navigation it is entirely at your own risk. I used it successfully for the 2016 Eco-Endurance Challenge but also had a Garmin GPS receiver and map and compass.


