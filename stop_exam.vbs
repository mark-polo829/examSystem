' 一键停止在线考试系统相关进程

Set shell = CreateObject("WScript.Shell")

' /F 强制结束，/IM 按映像名称结束
shell.Run "taskkill /F /IM ExamServer.exe", 0, True
shell.Run "taskkill /F /IM OnlineExamClient.exe", 0, True

MsgBox "在线考试系统已停止", vbInformation, "停止完成"
