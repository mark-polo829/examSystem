' 一键启动在线考试系统
' 服务器在后台无窗口运行，客户端正常显示

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

' 获取本脚本所在目录（项目根目录）
projectDir = fso.GetParentFolderName(WScript.ScriptFullName)

' 设置运行所需的 Qt / MinGW 环境
qtBin = "E:\qt\qt\5.14.2\mingw73_64\bin"
mingwBin = "E:\qt\qt\Tools\mingw730_64\bin"
shell.Environment("Process").Item("PATH") = qtBin & ";" & mingwBin & ";" & shell.Environment("Process").Item("PATH")

serverExe = fso.BuildPath(projectDir, "build\server\ExamServer.exe")
clientExe = fso.BuildPath(projectDir, "build\client\OnlineExamClient.exe")

If Not fso.FileExists(serverExe) Then
    MsgBox "未找到服务器程序：" & serverExe, vbCritical, "启动失败"
    WScript.Quit 1
End If

If Not fso.FileExists(clientExe) Then
    MsgBox "未找到客户端程序：" & clientExe, vbCritical, "启动失败"
    WScript.Quit 1
End If

' 0 = 隐藏窗口，False = 不等待其结束
shell.Run """" & serverExe & """", 0, False

' 等待服务器初始化（监听端口）
WScript.Sleep 1500

' 正常窗口启动客户端
shell.Run """" & clientExe & """", 1, False
