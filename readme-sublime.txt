
To do a build in Sublime using xctool, xctool must be in the path. If it isn't in the 
launchctl path, one way to add it is in Sublime's Python console, eg -

os.environ["PATH"] = os.environ["PATH"]+":/Users/dp/local/bin"

xcodebuild works as well, but xctool is much more user friendly.
