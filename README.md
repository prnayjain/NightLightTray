# NightLightTray

Changing the redness level of the Night Light feature on Windows requires you to navigate to the Night Light settings page through the control panel. I decided to make an old school tray icon to control it instead.  

<ul>
<li>Left click on the icon to bring up the redness meter.</li>
<li>Right click on the icon to toggle Night Light or exit the app.</li>
</ul>
  
There is a ready-to-use 32-bit executable in the top level directory.  

A known problem is that a guid for the tray program is hard coded and windows associates a particular guid with the program and the path it is first executed from. So please put the exec at the place you want it finally before executing it! Otherwise, just change the guid [here](https://github.com/prnayjain/NightLightTray/blob/master/NightLightTray.cpp#L29) and build another one for yourself.
