Dim WshShell
Set WshShell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
WshShell.Run """" & fso.GetParentFolderName(wscript.ScriptFullName) & "\RunS.bat""", 0, False
Set WshShell = Nothing