This is a command line program, but you can simply drag and drop a .chart file on the program and the converted MIDI file will be written to the same folder as the input file.  Any more elaborate use such as batch conversion would require some more complicated scripting or commands.  The command line usage of chartconvert is as follows:

chartconvert {input_filename.chart} [output_filename.mid] [-o]

The input filename is mandatory and of course is the .chart file that is to be converted (use quotation marks if the path has spaces)
The output filename is optional, in case you want to change the output folder or file name (use quotation marks if the path has spaces).  By default, the output filename is [input_filename].mid
The -o paramter is optional, and will allow chartconvert to overwrite the target file if it already exists.  By default, the conversion will cancel instead of overwrite the file.


***
**Example usage to convert all .chart files within a folder (including subfolders) to MIDI, without overwriting any existing MIDI files that may exist:

Due to the way Windows works, clicking and dragging multiple .chart files at a time onto the executable in order to convert many simultaneously may not work reliably.  Workarounds include using a script created for this purpose or using a suitable Windows command line utility like forfiles.  To use forfiles to convert all .chart files within a folder and all subfolders, put chartconvert's executable in any of the folders in your PATH environment variable (ie. open a command prompt, type the word set and hit enter to see the list of folders in the "Path=" part of the output.  Or you can add a folder containing chartconvert to the PATH variable if you know how to do that carefully.

Then open a command prompt and enter this command:
forfiles /p c:\ /m *.chart /s /c "cmd /c chartconvert.exe @file > nul"
This will search for all .chart files in all subfolders starting at c:\ (you can change this to some other path) and for each .chart file, passes it to chartconvert (which it finds automatically because it's in the PATH environment variable).  If you want to keep the console output, such as to redirect it to a file to review for failure messages, you can change nul to a filename, ie. output_log.txt.  This may slow it down some, but know that if you take out the > portion entirely all output will display to the console window and this will substantially slow down the conversion of each file.

You can likely skip putting chartconvert in the scope of the PATH variable if you change the working directory of the command prompt to the folder containing chartconvert.exe before issuing the forfiles command, but I haven't tested that.
