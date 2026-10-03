; Exercises the M4e Date library: Date (construct-from-components and
; current-time forms), Year/Month/Day/Hour/Minute/Second/DayOfWeek,
; FormatDate, AddDate (Day/Month/Year/Hour units, genuine calendar-
; normalized arithmetic via mktime, not a fixed day-offset hack), and the
; #PB_Date_* constants. Deliberately prints only component extractions, not
; the raw epoch integer itself - Date()/Year()/etc. interpret components as
; *local* time on both the construct and extract sides, so the round trip
; is identical regardless of the system's timezone, but the raw epoch
; VALUE itself is not, so it's never asserted directly here.
d.i = Date(2024, 3, 15, 10, 30, 45)
Debug Year(d)
Debug Month(d)
Debug Day(d)
Debug Hour(d)
Debug Minute(d)
Debug Second(d)
Debug DayOfWeek(d)
Debug FormatDate("%yyyy-%mm-%dd %hh:%ii:%ss", d)

tomorrow.i = AddDate(d, #PB_Date_Day, 1)
Debug Day(tomorrow)
Debug Month(tomorrow)

nextMonth.i = AddDate(d, #PB_Date_Month, 1)
Debug Month(nextMonth)
Debug Day(nextMonth)

nextYear.i = AddDate(d, #PB_Date_Year, 1)
Debug Year(nextYear)
Debug Month(nextYear)
Debug Day(nextYear)

plusHours.i = AddDate(d, #PB_Date_Hour, 2)
Debug Hour(plusHours)

Debug #PB_Date_Year
Debug #PB_Date_Month
Debug #PB_Date_Week
Debug #PB_Date_Day
Debug #PB_Date_Hour
Debug #PB_Date_Minute
Debug #PB_Date_Second

n.i = Date()
If n > 0
  Debug 1
Else
  Debug 0
EndIf
