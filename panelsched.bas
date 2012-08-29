Attribute VB_Name = "Panelist_Scheduler"
Private Declare PtrSafe Function GetExitCodeProcess Lib "kernel32" _
    (ByVal hProcess As Long, lpExitCode As Long) As Long
Private Declare PtrSafe Function WaitForSingleObject Lib "kernel32" _
    (ByVal hHandle As Long, ByVal dwMilliseconds As Long) As Long
Private Declare PtrSafe Function OpenProcess Lib "kernel32" _
    (ByVal dwDesiredAccess As Long, ByVal bInheritHandle As Long, _
    ByVal dwProcessId As Long) As Long
Private Declare PtrSafe Function CloseHandle Lib "kernel32" _
    (ByVal hObject As Long) As Long

Public Const INFINITE = &HFFFF
Public Const PROCESS_ALL_ACCESS = &H1F0FFF

Sub Schedule()
    progName = """" & Environ("APPDATA") & "\panelsched.exe"""

    wbName = ActiveWorkbook.Name
    wbPath = ActiveWorkbook.FullName
    stName = ActiveSheet.Name

    Application.DisplayAlerts = False
    Sheets(stName).Copy
    ActiveWorkbook.SaveAs Filename:=wbPath & ".csv", FileFormat _
        :=xlCSV, CreateBackup:=False, AddToMru:=False
    ActiveWorkbook.Close

    lTaskID = Shell(progName & " """ & wbPath & ".csv""", vbNormalFocus)
    lPID = OpenProcess(PROCESS_ALL_ACCESS, True, lTaskID)
    If lPID Then
        Call WaitForSingleObject(lPID, INFINITE)
        If GetExitCodeProcess(lPID, lExitCode) Then
            Select Case lExitCode
                Case 1
                    Kill wbPath & ".csv"
                    MsgBox "Unable to open CSV file for reading or writing"
                    Exit Sub
                Case 2
                    Kill wbPath & ".csv"
                    MsgBox "No solution found"
                    Exit Sub
            End Select
        Else
            Kill wbPath & ".csv"
            MsgBox "Error running scheduler"
            Exit Sub
        End If
    Else
        Kill wbPath & ".csv"
        MsgBox "Error running scheduler"
        Exit Sub
    End If
    CloseHandle (lPID)

    Workbooks.Open Filename:=wbPath & ".csv"
    wbNameCSV = ActiveWorkbook.Name
    Application.DisplayAlerts = True

    Workbooks(wbName).Activate
    Sheets(stName).Copy After:=Sheets(stName)
    ActiveSheet.Name = stName & " (scheduled)"

    Workbooks(wbNameCSV).Activate
    ActiveSheet.UsedRange.Select
    Selection.Copy
    Workbooks(wbName).Activate
    ActiveSheet.UsedRange.Select
    Selection.PasteSpecial Paste:=xlPasteAllExceptBorders, Operation:=xlNone, _
        SkipBlanks:=False, Transpose:=False
    Cells(1, 1).Select

    Application.DisplayAlerts = False
    Workbooks(wbNameCSV).Close
    Kill wbPath & ".csv"
    Application.DisplayAlerts = True
End Sub
