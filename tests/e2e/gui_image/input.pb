; Exercises M7b's fifth GUI slice: the Image library (CreateImage/
; LoadImage/IsImage/FreeImage/ImageID/ImageWidth/ImageHeight), plus
; CreateImageMenu and MenuItem/OpenSubMenu's own optional ImageID argument.
; test.bmp (a tiny 4x3 BMP, shipped alongside this test - see
; run_gui_case.sh's own "extra fixture files" copy step) stands in for a
; real PB example data file, since this project's own repo can't depend on
; an external PureBasic install's example assets.
;
; Deliberately avoids anything requiring a simulated click (xdotool) - the
; image-attachment itself is a purely visual effect with nothing to Debug-
; print, so this only checks return values/accessor round-trips, the same
; split gui_menu_statusbar's own test already uses.
r1 = CreateImage(0, 64, 32)
Debug r1
Debug IsImage(0)
Debug ImageWidth(0)
Debug ImageHeight(0)

r2 = LoadImage(1, "test.bmp")
Debug r2
Debug IsImage(1)
Debug ImageWidth(1)
Debug ImageHeight(1)

Debug IsImage(999) ; never created

If ImageID(0) <> 0
  Debug "ImageID(0) nonzero"
EndIf
If ImageID(1) <> 0
  Debug "ImageID(1) nonzero"
EndIf

; A bad filename is a harmless failure, not a crash.
r3 = LoadImage(2, "/nonexistent/path/does_not_exist.bmp")
Debug r3
Debug IsImage(2)

FreeImage(0)
Debug IsImage(0)

CreateImage(3, 4, 4)
CreateImage(4, 4, 4)
FreeImage(#PB_All)
Debug IsImage(1)
Debug IsImage(3)
Debug IsImage(4)

; CreateImageMenu + MenuItem/OpenSubMenu's own optional ImageID - re-create
; image 1 (just freed above) for this.
LoadImage(1, "test.bmp")
If OpenWindow(0, 0, 0, 220, 120, "img menu")
  r4 = CreateImageMenu(0, WindowID(0))
  Debug r4
  MenuTitle("Project")
  MenuItem(10, "Open", ImageID(1))
  MenuItem(11, "Close")
  MenuBar()
  OpenSubMenu("Recent", ImageID(1))
  MenuItem(12, "file.txt")
  CloseSubMenu()
  Debug IsMenu(0)
  Debug GetMenuItemText(0, 10)
  CloseWindow(0)
EndIf
