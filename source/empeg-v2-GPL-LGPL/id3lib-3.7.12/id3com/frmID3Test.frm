VERSION 5.00
Begin VB.Form frmID3Test 
   Caption         =   "ID3 Test"
   ClientHeight    =   3195
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Command1"
      Height          =   615
      Left            =   960
      TabIndex        =   0
      Top             =   480
      Width           =   2055
   End
End
Attribute VB_Name = "frmID3Test"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Command1_Click()
    Dim oTag As New ID3Tag
    Dim oFrame As ID3Frame
    Dim oField As ID3Field
    Dim n As Long
    '230-picture
    'oTag.Link "c:\source\downloads\230-picture.tag"
    oTag.Link "c:\source\downloads\len - steal my sunshine.mp3"
    For Each oFrame In oTag
        Debug.Print oFrame.FrameName
        Set oField = oFrame.Field(ID3_FIELD_TEXT)
        If Not oField Is Nothing Then
            For n = 1 To oField.NumTextItems
                Debug.Print oField.Text(n)
            Next n
        End If
        Set oField = oFrame.Field(ID3_FIELD_DATA)
        If Not oField Is Nothing Then
            oField.CopyDataToFile "c:\source\downloads\230-picture.jpg"
        End If
    Next
    oTag.LastPlayed = Now
    Debug.Print oTag.LastPlayed
    If oTag.HasV1Tags Then
        oTag.StripV1Tag
    End If
    oTag.SaveV2Tag
End Sub

