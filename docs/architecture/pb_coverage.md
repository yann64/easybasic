# PureBasic procedure coverage

Generated from the real, locally-installed PureBasic compiler (`pbcompilerc -lf` for the full procedure list with signatures, cross-referenced against each library's own internal definition file under `purebasic/purelibraries/` for library grouping), compared against this project's own implemented builtins (`compiler/src/sema/sema.cpp`'s builtin signature tables).

**Regenerating this table**: `python3 tools/pb_coverage/generate.py` from the repo root (needs `pbcompilerc` on PATH, or pass `--pbcompilerc`/`--purelibraries` explicitly) - rewrites this file directly. Run it after any slice that adds or removes a builtin.

**Summary**: **264 / 1848** procedures implemented (14%).

5 names (`Bool`, `Defined`, `OffsetOf`, `SizeOf`, `Subsystem`) are
compile-time/language-level constructs, not real library procedures - found in no
library's own definition file at all; listed in their own `Language/Compiler` group.

## By library

| Library | Implemented | Total | % |
|---|---:|---:|---:|
| [2DDrawing](#2ddrawing) | 17 | 42 | 40% |
| [2DDrawingBase](#2ddrawingbase) | 6 | 7 | 85% |
| [Array](#array) | 0 | 6 | 0% |
| [AudioCD](#audiocd) | 0 | 13 | 0% |
| [Billboard](#billboard) | 0 | 25 | 0% |
| [Camera](#camera) | 0 | 34 | 0% |
| [CGI](#cgi) | 0 | 21 | 0% |
| [Cipher](#cipher) | 0 | 22 | 0% |
| [CipherBCrypt](#cipherbcrypt) | 0 | 2 | 0% |
| [CipherCRC32](#ciphercrc32) | 0 | 1 | 0% |
| [CipherMD5](#ciphermd5) | 0 | 1 | 0% |
| [CipherSHA1](#ciphersha1) | 0 | 1 | 0% |
| [CipherSHA2](#ciphersha2) | 0 | 1 | 0% |
| [CipherSHA3](#ciphersha3) | 0 | 1 | 0% |
| [Clipboard](#clipboard) | 0 | 3 | 0% |
| [ClipboardImage](#clipboardimage) | 0 | 2 | 0% |
| [Console](#console) | 0 | 16 | 0% |
| [Database](#database) | 0 | 32 | 0% |
| [DatabaseMySQL](#databasemysql) | 0 | 1 | 0% |
| [DatabaseODBC](#databaseodbc) | 0 | 5 | 0% |
| [DatabasePostgreSQL](#databasepostgresql) | 0 | 1 | 0% |
| [DatabaseSQLite](#databasesqlite) | 0 | 1 | 0% |
| [Date](#date) | 10 | 14 | 71% |
| [DebuggerFunctions](#debuggerfunctions) | 0 | 18 | 0% |
| [Desktop](#desktop) | 0 | 16 | 0% |
| [Dialog](#dialog) | 0 | 10 | 0% |
| [DialogOpenGL](#dialogopengl) | 0 | 1 | 0% |
| [DialogScintilla](#dialogscintilla) | 0 | 1 | 0% |
| [DialogWeb](#dialogweb) | 0 | 1 | 0% |
| [DialogWebView](#dialogwebview) | 0 | 1 | 0% |
| [DragDrop](#dragdrop) | 0 | 22 | 0% |
| [Engine3D](#engine3d) | 0 | 4 | 0% |
| [Entity](#entity) | 0 | 59 | 0% |
| [EntityAnimation](#entityanimation) | 0 | 12 | 0% |
| [Event](#event) | 4 | 10 | 40% |
| [File](#file) | 11 | 40 | 27% |
| [FileSystem](#filesystem) | 3 | 29 | 10% |
| [Font](#font) | 0 | 5 | 0% |
| [Ftp](#ftp) | 0 | 23 | 0% |
| [Gadget](#gadget) | 68 | 82 | 82% |
| [GadgetOpenGL](#gadgetopengl) | 0 | 1 | 0% |
| [GadgetWeb](#gadgetweb) | 0 | 1 | 0% |
| [Help](#help) | 0 | 2 | 0% |
| [HID](#hid) | 0 | 17 | 0% |
| [Http](#http) | 0 | 15 | 0% |
| [Image](#image) | 7 | 24 | 29% |
| [ImagePluginGIF](#imageplugingif) | 0 | 1 | 0% |
| [ImagePluginJPEG](#imagepluginjpeg) | 0 | 2 | 0% |
| [ImagePluginJPEG2000](#imagepluginjpeg2000) | 0 | 2 | 0% |
| [ImagePluginOS](#imagepluginos) | 0 | 2 | 0% |
| [ImagePluginPNG](#imagepluginpng) | 0 | 2 | 0% |
| [ImagePluginTGA](#imageplugintga) | 0 | 1 | 0% |
| [ImagePluginTIFF](#imageplugintiff) | 0 | 1 | 0% |
| [ImagePluginWebP](#imagepluginwebp) | 0 | 2 | 0% |
| [Joint](#joint) | 0 | 11 | 0% |
| [Joystick](#joystick) | 0 | 7 | 0% |
| [Json](#json) | 0 | 53 | 0% |
| [Keyboard](#keyboard) | 0 | 6 | 0% |
| [Language/Compiler](#language-compiler) | 0 | 5 | 0% |
| [Library](#library) | 0 | 14 | 0% |
| [Light](#light) | 0 | 24 | 0% |
| [LinkedList](#linkedlist) | 0 | 24 | 0% |
| [Mail](#mail) | 0 | 13 | 0% |
| [Map](#map) | 0 | 13 | 0% |
| [Material](#material) | 0 | 33 | 0% |
| [Math](#math) | 17 | 33 | 51% |
| [Math3D](#math3d) | 0 | 12 | 0% |
| [Memory](#memory) | 20 | 40 | 50% |
| [Menu](#menu) | 21 | 24 | 87% |
| [Mesh](#mesh) | 0 | 46 | 0% |
| [Mouse](#mouse) | 0 | 10 | 0% |
| [Movie](#movie) | 0 | 16 | 0% |
| [Music](#music) | 0 | 10 | 0% |
| [Network](#network) | 0 | 23 | 0% |
| [Node](#node) | 0 | 17 | 0% |
| [NodeAnimation](#nodeanimation) | 0 | 23 | 0% |
| [OnError](#onerror) | 0 | 16 | 0% |
| [Packer](#packer) | 0 | 19 | 0% |
| [PackerBriefLZ](#packerbrieflz) | 0 | 1 | 0% |
| [PackerJCALG1](#packerjcalg1) | 0 | 1 | 0% |
| [PackerLZMA](#packerlzma) | 0 | 1 | 0% |
| [PackerTAR](#packertar) | 0 | 1 | 0% |
| [PackerZip](#packerzip) | 0 | 1 | 0% |
| [Particle](#particle) | 0 | 25 | 0% |
| [Preference](#preference) | 0 | 27 | 0% |
| [Printer](#printer) | 0 | 9 | 0% |
| [Process](#process) | 0 | 25 | 0% |
| [RegularExpression](#regularexpression) | 0 | 19 | 0% |
| [Requester](#requester) | 13 | 13 | 100% |
| [Runtime](#runtime) | 0 | 7 | 0% |
| [Scintilla](#scintilla) | 0 | 2 | 0% |
| [Screen](#screen) | 0 | 19 | 0% |
| [ScreenGadget](#screengadget) | 0 | 54 | 0% |
| [SerialPort](#serialport) | 0 | 13 | 0% |
| [Skeleton](#skeleton) | 0 | 6 | 0% |
| [Sort](#sort) | 0 | 9 | 0% |
| [Sound](#sound) | 0 | 19 | 0% |
| [Sound3D](#sound3d) | 0 | 10 | 0% |
| [SoundPluginFLAC](#soundpluginflac) | 0 | 2 | 0% |
| [SoundPluginMP3](#soundpluginmp3) | 0 | 2 | 0% |
| [SoundPluginOGG](#soundpluginogg) | 0 | 2 | 0% |
| [SoundPluginOpus](#soundpluginopus) | 0 | 2 | 0% |
| [SpecialEffect](#specialeffect) | 0 | 12 | 0% |
| [Spline](#spline) | 0 | 13 | 0% |
| [Sprite](#sprite) | 0 | 25 | 0% |
| [StaticGeometry](#staticgeometry) | 0 | 5 | 0% |
| [StatusBar](#statusbar) | 7 | 9 | 77% |
| [String](#string) | 15 | 37 | 40% |
| [StringBuilder](#stringbuilder) | 0 | 7 | 0% |
| [System](#system) | 2 | 9 | 22% |
| [SystemGtk](#systemgtk) | 0 | 1 | 0% |
| [SysTray](#systray) | 6 | 6 | 100% |
| [Terrain](#terrain) | 0 | 24 | 0% |
| [Text3D](#text3d) | 0 | 12 | 0% |
| [Texture](#texture) | 0 | 16 | 0% |
| [Thread](#thread) | 17 | 18 | 94% |
| [ToolBar](#toolbar) | 12 | 12 | 100% |
| [VectorDrawing](#vectordrawing) | 0 | 66 | 0% |
| [VectorDrawingSvg](#vectordrawingsvg) | 0 | 1 | 0% |
| [Vehicle](#vehicle) | 0 | 8 | 0% |
| [VertexAnimation](#vertexanimation) | 0 | 8 | 0% |
| [WebView](#webview) | 0 | 5 | 0% |
| [Window](#window) | 8 | 40 | 20% |
| [World](#world) | 0 | 36 | 0% |
| [Xml](#xml) | 0 | 57 | 0% |

## 2DDrawing

17/42 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `BackColor` | (Color) - Set the default background color for graphic functions and text display. |
| Yes | `Box` | (x, y, Width, Height [, Color]) - Draw a filled box of given dimensions on the current output. |
| No | `BoxedGradient` | (x, y, Width, Height) - Use a box shaped gradient when gradient drawing is enabled. |
| Yes | `Circle` | (x, y, Radius [, Color]) - Draw a filled circle on the current output. |
| No | `CircularGradient` | (x, y, Radius) - Use a circular gradient when gradient drawing is enabled. |
| No | `ClipOutput` | (x, y, Width, Height) - Restrict drawing output to a clipping rectangle. |
| No | `ConicalGradient` | (x, y, Angle.f) - Use a conical gradient when gradient drawing is enabled. |
| No | `CustomFilterCallback` | (@Callback()) - Set a callback for the #PB_2DDrawing_CustomFilter mode. |
| No | `CustomGradient` | (@Callback()) - Use a callback procedure to define the gradient if gradient drawing is enabled. |
| No | `DrawAlphaImage` | (ImageID, x, y [, ConstAlpha]) - Display the image on the current output, using the alpha information. |
| No | `DrawImage` | (ImageID, x, y [, Width, Height [, Mode]]) - Display the image on the current output. On the fly resize is possible. |
| No | `DrawingBuffer` | () - Return the current drawing memory buffer. |
| No | `DrawingBufferPitch` | () - Return the number of bytes needed for a full line. |
| No | `DrawingBufferPixelFormat` | () - Return the pixel format of the current drawing buffer. |
| No | `DrawingFont` | (FontID()) - Set the current font to given FontID. |
| Yes | `DrawingMode` | (Mode) - Change the drawing mode for graphic and text output. |
| No | `DrawRotatedText` | (x.d, y.d, Text$, Angle.f [, Color]) - Display the given string at the given angle. |
| Yes | `DrawText` | (x.d, y.d, Text$ [, FrontColor [, BackColor]]) - Display the given string on the current output. |
| Yes | `Ellipse` | (x, y, RadiusX, RadiusY [, Color]) - Draw a filled ellipse on the current output. |
| No | `EllipticalGradient` | (x, y, RadiusX, RadiusY) - Use an elliptical gradient when gradient drawing is enabled. |
| Yes | `FillArea` | (x, y, OutlineColor [, FillColor]) - Fill a shape with the specified color. |
| Yes | `FrontColor` | (Color) - Set the default color for graphic functions and text display. |
| No | `GetOriginX` | () - Get the x coordinate of the drawing origin. |
| No | `GetOriginY` | () - Get the y coordinate of the drawing origin. |
| No | `GrabDrawingImage` | (#Image, x, y, Width, Height) - Create a new #Image with the content of the given drawing output area. |
| No | `GradientColor` | (Position.f, Color) - Set a color for the drawing gradient. |
| Yes | `Line` | (x, y, Width, Height [, Color]) - Draw a line of given dimension on the current output. |
| No | `LinearGradient` | (x1, y1, x2, y2) - Use a linear gradient when gradient drawing is enabled. |
| Yes | `LineXY` | (x1, y1, x2, y2 [, Color]) - Draw a line of given dimension on the current output. |
| No | `OutputDepth` | () - Get the depth of the current drawing output. |
| No | `OutputHeight` | () - Get the height of the current drawing output. |
| No | `OutputWidth` | () - Get the width of the current drawing output. |
| Yes | `Plot` | (x, y [, Color]) - Draw a plot at the given coordinates in the current output. |
| Yes | `Point` | (x, y) - Return the color used at the coordinates (x,y) in the current output. |
| No | `ResetGradientColors` | () - Clear all colors from the drawing gradient and reset it to BackColor->FrontColor. |
| Yes | `RoundBox` | (x, y, Width, Height, RoundX, RoundY [, Color]) - Draw a box with rounded edges on the current output. |
| No | `SetOrigin` | (x, y) - Set the coordinate origin for drawing operations. |
| Yes | `StartDrawing` | (Output) - Change the current drawing output to the specified output. |
| Yes | `StopDrawing` | () - Must be called after a StartDrawing(), when all graphics operations are finished. |
| Yes | `TextHeight` | (Text$) - Return the height (in pixel) of the given string in the current output using the current font. |
| Yes | `TextWidth` | (Text$) - Return the length (in pixel) of the given string in the current output using the current font. |
| No | `UnclipOutput` | () - Remove any clipping from the drawing output. |

## 2DDrawingBase

6/7 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `Alpha` | (Color) - Return the alpha component of the given color. |
| No | `AlphaBlend` | (Color1, Color2) - Return the result of blending Color1 on Color2 with alphachannel. |
| Yes | `Blue` | (Color) - Return the blue component of the given color. |
| Yes | `Green` | (Color) - Return the green component of the given color. |
| Yes | `Red` | (Color) - Return the red component of the given color. |
| Yes | `RGB` | (Red, Green, Blue) - Return the 24 bits color corresponding to the given RGB parameters. |
| Yes | `RGBA` | (Red, Green, Blue, Alpha) - Return the 32 bits color corresponding to the given RGBA parameters. |

## Array

0/6 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ArraySize` | (Array() [, Dimension]) - Return the array size. |
| No | `CompareArray` | (Array1(), Array2() [, Flags]) - Compare both arrays and return 1 if they are the same or 0 if an element differs. |
| No | `CopyArray` | (SourceArray(), DestinationArray()) - Copy all the elements from the array to the destination array to have two identical arrays. |
| No | `ExtractArray` | (Array(), Text$ [, Separator$ [, Limit]]) - Split 'Text$' into 'Array()' using 'Separator$' as the field separator (default ",") and return the number of elements found. If 'Limit' is greater than zero, at most 'Limit' elements are extracted, the last one containing the remainder of the string. 'Array()' is redimensioned to the exact number of elements found, and has to be of String, Integer or Double type. |
| No | `FreeArray` | (Array()) - Free the array. 'Dim' has to be called to use it again. |
| No | `JoinArray` | (Array() [, Separator$ [, Limit]]) - Join all the elements of 'Array()' into a single string, using 'Separator$' between each element (default ","). If 'Limit' is greater than zero, only the first 'Limit' elements are joined. 'Array()' has to be of String, Integer or Double type. |

## AudioCD

0/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AudioCDLength` | () - Return the full length of a CD-Audio in seconds. |
| No | `AudioCDName` | () - Return the name, system dependent, attached to the current CD-Audio drive. |
| No | `AudioCDStatus` | () - Possible values: -1 Ready; 0 Stopped; >0 Audio track which is currently playing. |
| No | `AudioCDTrackLength` | (TrackNumber) - Return the length of the specified track, in seconds. |
| No | `AudioCDTracks` | () - Return the total number of tracks on the CD available for playing. |
| No | `AudioCDTrackSeconds` | () - Return the number of seconds elapsed since the current track has started playing. |
| No | `EjectAudioCD` | (State) - Eject (open) or close the current CD-Drive. |
| No | `InitAudioCD` | () - Initialize all CD-Audio resources. |
| No | `PauseAudioCD` | () - Pause the AudioCD playback. |
| No | `PlayAudioCD` | (StartTrack, EndTrack) - Start to play the CD-Audio from 'StartTrack' until the end of 'EndTrack'. |
| No | `ResumeAudioCD` | () - Resume the AudioCD playback. |
| No | `StopAudioCD` | () - Stop the current CD-Audio playback. |
| No | `UseAudioCD` | (AudioCDDrive) - Change the current AudioCD drive. |

## Billboard

0/25 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddBillboard` | (#BillboardGroup, x, y, z) - Add a billboard to the #BillboardGroup. |
| No | `BillboardGroupCommonDirection` | (#BillboardGroup, x, y, z) - Set the billboardgroup common direction. |
| No | `BillboardGroupCommonUpVector` | (#BillboardGroup, x, y, z) - Set the billboardgroup common up vector. |
| No | `BillboardGroupID` | (#BillboardGroup) - Return the unique ID which identifies the billboard group in the operating system. |
| No | `BillboardGroupMaterial` | (#BillboardGroup, MaterialID) - Change the #BillboardGroup material used by all the billboards. |
| No | `BillboardGroupX` | (#BillboardGroup [, Mode]) - Return the #BillboardGroup X absolute position. |
| No | `BillboardGroupY` | (#BillboardGroup [, Mode]) - Return the #BillboardGroup Y absolute position. |
| No | `BillboardGroupZ` | (#BillboardGroup [, Mode]) - Return the #BillboardGroup Z absolute position. |
| No | `BillboardHeight` | (#Billboard, #BillboardGroup) - Return the #Billboard height, in world metric. |
| No | `BillboardLocate` | (#Billboard, #BillboardGroup, x, y, z) - Change the #Billboard absolute location, relative to the #BillboardGroup. |
| No | `BillboardWidth` | (#Billboard, #BillboardGroup) - Return the #Billboard width, in world metric. |
| No | `BillboardX` | (#Billboard, #BillboardGroup) - Return the #Billboard X absolute position in the #BillboardGroup. |
| No | `BillboardY` | (#Billboard, #BillboardGroup) - Return the #Billboard Y absolute position in the #BillboardGroup. |
| No | `BillboardZ` | (#Billboard, #BillboardGroup) - Return the #Billboard Z absolute position in the #BillboardGroup. |
| No | `ClearBillboards` | (#BillboardGroup) - Remove and destroy all the billboards of the #BillboardGroup. |
| No | `CountBillboards` | (#BillboardGroup) - Count the number of billboards contained in the #BillboardGroup. |
| No | `CreateBillboardGroup` | (#BillboardGroup, MaterialID, DefaultBillboardWidth, DefaultBillboardHeight, [x, y, z [, VisibilityMask [, Type]]]) - Create a new #BillboardGroup. |
| No | `FreeBillboardGroup` | (#BillboardGroup) - Free the specified #BillboardGroup and all its billboards. |
| No | `HideBillboardGroup` | (#BillboardGroup, Display) - Hide or show the #BillboardGroup and all its billboards. |
| No | `IsBillboardGroup` | (#BillboardGroup) - Test if the given '#BillboardGroup' is an initialized BillboardGroup object. |
| No | `MoveBillboard` | (#Billboard, #BillboardGroup, x, y, z) - Move the #Billboard relative to its previous location. |
| No | `MoveBillboardGroup` | (#BillboardGroup, x, y, z [, Mode]) - Move the #BillboardGroup relative to its previous location. |
| No | `RemoveBillboard` | (#Billboard, #BillboardGroup) - Remove the specified #Billboard. |
| No | `ResizeBillboard` | (#Billboard, #BillboardGroup, Width, Height) - Change the #Billboard dimensions. |
| No | `RotateBillboardGroup` | (#BillboardGroup, x, y, z [, Mode]) - Rotate the #BillboardGroup. |

## Camera

0/34 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CameraBackColor` | (#Camera, Color) - Change the #Camera background color. |
| No | `CameraCustomParameter` | (#Camera, ParameterIndex, Value1, Value2, Value3, Value4) - Set a custom parameter to the #Entity shader script. |
| No | `CameraDirection` | (#Camera, x, y, z) - Change the #Camera direction. |
| No | `CameraDirectionX` | (#Camera [, Mode]) - Retrieve the x direction of the #Camera. |
| No | `CameraDirectionY` | (#Camera [, Mode]) - Retrieve the y direction of the #Camera. |
| No | `CameraDirectionZ` | (#Camera [, Mode]) - Retrieve the z direction of the #Camera. |
| No | `CameraFixedYawAxis` | (#Camera, Mode [, VectorX, VectorY, VectorZ]) - Change the way the yaw axis is managed. |
| No | `CameraFollow` | (#Camera, ObjectID, Angle, Height, Distance, RotationPercent, PositionPercent [, Mode]) |
| No | `CameraFOV` | (#Camera, FOV) - Change the #Camera Field Of View (FOV). |
| No | `CameraID` | (#Camera) - Return the unique ID which identifies the camera in the operating system. |
| No | `CameraLookAt` | (#Camera, x, y, z) - Make the camera look at the x,y,z point. |
| No | `CameraPitch` | (#Camera [, Mode]) - Retrieve the pitch of the #Camera. |
| No | `CameraProjectionMode` | (#Camera, Mode [, Width, Height]) - Change the #Camera projection mode. |
| No | `CameraProjectionX` | (#Camera, x, y, z) - Return the X position, in pixels, of the 3D point. |
| No | `CameraProjectionY` | (#Camera, x, y, z) - Return the Y position, in pixels, of the 3D point. |
| No | `CameraRange` | (#Camera, Near, Far) - Change the #Camera range. |
| No | `CameraReflection` | (#Camera, #MainCamera, EntityID) - Update the #Camera used for the reflection with the #MainCamera. |
| No | `CameraRenderMode` | (#Camera, RenderMode) - Change the #Camera render mode. |
| No | `CameraRoll` | (#Camera [, Mode]) - Retrieve the roll of the #Camera. |
| No | `CameraViewHeight` | (#Camera) - Retrieve the height of the #Camera view, in pixels. |
| No | `CameraViewWidth` | (#Camera) - Retrieve the width of the #Camera view, in pixels. |
| No | `CameraViewX` | (#Camera) - Retrieve the x position on the screen of the #Camera view, in pixels. |
| No | `CameraViewY` | (#Camera) - Retrieve the y position on the screen of the #Camera view, in pixels. |
| No | `CameraX` | (#Camera [, Mode]) - Retrieve the x position of the #Camera. |
| No | `CameraY` | (#Camera [, Mode]) - Retrieve the y position of the #Camera. |
| No | `CameraYaw` | (#Camera [, Mode]) - Retrieve the yaw of the #Camera. |
| No | `CameraZ` | (#Camera [, Mode]) - Retrieve the z position of the #Camera. |
| No | `CheckObjectVisibility` | (#Camera, ObjectID) - Check if the object is currently visible on the specified camera. |
| No | `CreateCamera` | (#Camera, x, y, Width, Height [, VisibilityMask [, Lod]]) - Create a new #Camera. All the dimensions are in percent. |
| No | `FreeCamera` | (#Camera) - Free the specified #Camera. |
| No | `IsCamera` | (#Camera) - Test if the given '#Camera' is an initialized camera object. |
| No | `MoveCamera` | (#Camera, x, y, z [, Mode]) - Move the #Camera relative to its previous position. |
| No | `ResizeCamera` | (#Camera, x, y, Width, Height) - Resize the #Camera. All the dimensions are in percent. |
| No | `RotateCamera` | (#Camera, x, y, z [, Mode]) - Rotate the #Camera. |

## CGI

0/21 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CGIBuffer` | () - Get the CGI web content buffer pointer. |
| No | `CGICookieName` | (Index) - Get the specified CGI cookie name. |
| No | `CGICookieValue` | (Name$) - Get the specified CGI cookie value. |
| No | `CGIParameterData` | (Name$ [, Index]) - Get the specified CGI parameter data. |
| No | `CGIParameterDataSize` | (Name$ [, Index]) - Get the specified CGI parameter data size. |
| No | `CGIParameterName` | (Index) - Get the specified CGI parameter name. |
| No | `CGIParameterType` | (Name$ [, Index]) - Get the specified CGI parameter type. |
| No | `CGIParameterValue` | (Name$ [, Index]) - Get the specified CGI parameter value. |
| No | `CGIVariable` | (VariableName$) - Get the specified CGI variable content. |
| No | `CountCGICookies` | () - Get the number of cookies. |
| No | `CountCGIParameters` | () - Get the number of CGI parameters. |
| No | `FinishFastCGIRequest` | () - Finish the current FastCGI request. |
| No | `InitCGI` | ([MaxRequestSize]) - Init the CGI library. |
| No | `InitFastCGI` | (LocalPort [, BoundIP$]) - Init the FastCGI support. |
| No | `ReadCGI` | () - Read the CGI web content. |
| No | `WaitFastCGIRequest` | () - Wait for FastCGI request to process. |
| No | `WriteCGIData` | (*Buffer, Size) - Write binary data to CGI output. |
| No | `WriteCGIFile` | (Filename$) - Write a whole file to CGI output. |
| No | `WriteCGIHeader` | (Name$, Value$ [, Flags]) - Add a new header to the answer. |
| No | `WriteCGIString` | (String$ [, Encoding]) - Write a string to the CGI output. |
| No | `WriteCGIStringN` | (String$ [, Encoding]) - Write a string to the CGI output including a carriage return. |

## Cipher

0/22 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddCipherBuffer` | (#Cipher, *Input, *Output, Size) - Add a new buffer to the #Cipher. |
| No | `AddFingerprintBuffer` | (#Fingerprint, *Buffer, Size) - Add a new buffer to the fingerprint calculation. |
| No | `AESDecoder` | (*Input, *Output, Size, *Key, Bits, *InitializationVector [, Mode]) - Decode the specified buffer using the AES algorithm. |
| No | `AESEncoder` | (*Input, *Output, Size, *Key, Bits, *InitializationVector [, Mode]) - Encode the specified buffer using the AES algorithm. |
| No | `Base64Decoder` | (Input$, *Output, OutputSize) - Decode the specified buffer using the Base64 algorithm. |
| No | `Base64DecoderBuffer` | (*Input, InputSize, *Output, OutputSize) - Decode the specified buffer using the Base64 algorithm. |
| No | `Base64Encoder` | (*Input, InputSize [, Flags]) - Encode the specified buffer using the Base64 algorithm. |
| No | `Base64EncoderBuffer` | (*Input, InputSize, *Output, OutputSize [, Flags]) - Encode the specified buffer using the Base64 algorithm. |
| No | `CloseCryptRandom` | () - Close the cryptographic safe pseudorandom number generator and free its resources. |
| No | `CryptRandom` | (Maximum) - Return a random number from the cryptographic safe pseudorandom number generator. |
| No | `CryptRandomData` | (*Buffer, Length) - Fill a buffer with data from the cryptographic random number generator. |
| No | `DeriveCipherKey` | (Password$, Salt$, Iterations, *Key, Bits, Plugin [, PluginBits]) - Compute a cipher key from a password. |
| No | `FileFingerprint` | (Filename$ , Plugin [, Bits [, Offset [, Length [, Key$ [, KeyFormat]]]]]) - Return the fingerprint of the file. |
| No | `Fingerprint` | (*Buffer, Size, Plugin [, Bits [, Key$ [, KeyFormat]]]) - Return the fingerprint of the specified buffer. |
| No | `FinishCipher` | (#Cipher) - Free all the resources allocated by the #Cipher. |
| No | `FinishFingerprint` | (#Fingerprint) - Return the calculated fingerprint string and free all resources. |
| No | `IsCipher` | (#Cipher) - Test if the given '#Cipher' is an initialized cipher object. |
| No | `IsFingerprint` | (#Fingerprint) - Test if the given '#Fingerprint' is an initialized fingerprint object. |
| No | `OpenCryptRandom` | () - Open the cryptographic safe pseudorandom number generator. |
| No | `StartAESCipher` | (#Cipher, *Key, KeyBits, *InitializationVector, Mode) - Create a new streaming cipher. |
| No | `StartFingerprint` | (#Fingerprint, Plugin [, Bits [, Key$ [, KeyFormat]]]) - Initialize a step-by-step fingerprint calculation. |
| No | `StringFingerprint` | (String, Plugin [, Bits [, Format [, Key$ [, KeyFormat]]]]) - Return the fingerprint of the specified string. |

## CipherBCrypt

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CreatePasswordHash` | (Password$, [WorkFactor]) - Create a password hash for safe storage. |
| No | `VerifyPasswordHash` | (Password$, Hash$) - Verify a password against a previously created password hash. |

## CipherCRC32

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseCRC32Fingerprint` | () - Register the CRC32 fingerprint plugin for use with the cipher commands. |

## CipherMD5

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseMD5Fingerprint` | () - Register the MD5 fingerprint plugin for use with the cipher commands. |

## CipherSHA1

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseSHA1Fingerprint` | () - Register the SHA-1 fingerprint plugin for use with the cipher commands. |

## CipherSHA2

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseSHA2Fingerprint` | () - Register the SHA-2 fingerprint plugin for use with the cipher commands. |

## CipherSHA3

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseSHA3Fingerprint` | () - Register the SHA-3 fingerprint plugin for use with the cipher commands. |

## Clipboard

0/3 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ClearClipboard` | () - Clear the clipboard of its content. |
| No | `GetClipboardText` | () - Return the last text pasted in the clipboard. |
| No | `SetClipboardText` | (Text$) - Store the specified Text$ in the clipboard. |

## ClipboardImage

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `GetClipboardImage` | (#Image [, Depth]) - Get an image from the clipboard (if any). |
| No | `SetClipboardImage` | (#Image) - Set the specified #Image in the clipboard. |

## Console

0/16 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ClearConsole` | () - Clear the whole console content. |
| No | `CloseConsole` | () - Close the current console. |
| No | `ConsoleColor` | (CharacterColor, BackgroundColor) - Change the colors used by the text display. |
| No | `ConsoleCursor` | (Height) - Change the cursor display. |
| No | `ConsoleError` | (Message$) - Write a message to the standard error output. |
| No | `ConsoleLocate` | (x, y) - Move the cursor to the given position, in character coordinates. |
| No | `ConsoleTitle` | (Title$) - Change the console title to the new given one. |
| No | `EnableGraphicalConsole` | (State) - Enable or disable the graphical console. |
| No | `Inkey` | () - Return a two characters string if a key is pressed during the call of Inkey(). |
| No | `Input` | () - Catch a whole character line. |
| No | `OpenConsole` | ([Title$ [, Mode]]) - Open a console window. |
| No | `Print` | (Text$) - Display the Text$ in the console. |
| No | `PrintN` | (Text$) - Display the Text$ in the console and add a line return. |
| No | `RawKey` | () - Return the last raw key, after the last Inkey() call. |
| No | `ReadConsoleData` | (*Buffer, Size) - Read data from the console input. |
| No | `WriteConsoleData` | (*Buffer, Size) - Write data to the console output. |

## Database

0/32 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AffectedDatabaseRows` | (#Database) - Get number of rows affected by a database update. |
| No | `CheckDatabaseNull` | (#Database, Column) - Return #True if the specified column contains NULL. |
| No | `CloseDatabase` | (#Database) - Close the specified database. |
| No | `DatabaseColumnIndex` | (#Database, ColumnName$) - Return the zero-based index of the specified column name. |
| No | `DatabaseColumnName` | (#Database, Column) - Return the name of the specified column. |
| No | `DatabaseColumns` | (#Database) - Return the numbers of columns (fields) in the specified opened database. |
| No | `DatabaseColumnSize` | (#Database, Column) - Return the size of the specified column. |
| No | `DatabaseColumnType` | (#Database, Column) - Return the type of the specified column. |
| No | `DatabaseError` | () - Return last error (if any) in human readable form. |
| No | `DatabaseID` | (#Database) - Return the database handle for direct access with the related API. |
| No | `DatabaseQuery` | (#Database, Request$ [, Flags]) - Execute the SQL Request$ on the specified database. |
| No | `DatabaseUpdate` | (#Database, Request$) - Execute the SQL Request$ on the specified database (update mode). |
| No | `FinishDatabaseQuery` | (#Database) - Finish a database query and release associated memory. |
| No | `FirstDatabaseRow` | (#Database) - Retrieve information about the first database row. |
| No | `GetDatabaseBlob` | (#Database, Column, *Buffer, BufferLength) - Return the content of the specified column in the specified buffer. |
| No | `GetDatabaseDouble` | (#Database, Column) - Return the content of the specified column as a double number. |
| No | `GetDatabaseFloat` | (#Database, Column) - Return the content of the specified column as a float number. |
| No | `GetDatabaseLong` | (#Database, Column) - Return the content of the specified column as a standard number. |
| No | `GetDatabaseQuad` | (#Database, Column) - Return the content of the specified column as a quad number. |
| No | `GetDatabaseString` | (#Database, Column) - Return the content of the specified column as a string. |
| No | `IsDatabase` | (#Database) - Test if the given '#Database' is an initialized database object. |
| No | `NextDatabaseRow` | (#Database) - Retrieve information about the next database row. |
| No | `OpenDatabase` | (#Database, DatabaseName$, User$, Password$ [, Plugin]) - Open the specified database. |
| No | `OpenDatabaseRequester` | (#Database [, Plugin]) - Open the standard Windows ODBC requester to choose which database to open. |
| No | `PreviousDatabaseRow` | (#Database) - Retrieve information about the previous database row. |
| No | `SetDatabaseBlob` | (#Database, StatementIndex, *Buffer, BufferLength) - Set a blob as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseDouble` | (#Database, StatementIndex, Value.d) - Set a double as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseFloat` | (#Database, StatementIndex, Value.f) - Set a float as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseLong` | (#Database, StatementIndex, Value) - Set a number as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseNull` | (#Database, StatementIndex) - Set a NULL value as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseQuad` | (#Database, StatementIndex, Value.q) - Set a quad number as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |
| No | `SetDatabaseString` | (#Database, StatementIndex, Value$) - Set a string as a bind variable for the next DatabaseQuery() or DatabaseUpdate(). |

## DatabaseMySQL

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseMySQLDatabase` | ([LibraryName$]) - Add MySQL support to the database commandset. |

## DatabaseODBC

0/5 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `DatabaseDriverDescription` | () - Return the description of the current database driver. |
| No | `DatabaseDriverName` | () - Return the name of the current database driver. |
| No | `ExamineDatabaseDrivers` | () - Examine the installed database drivers available on the system. |
| No | `NextDatabaseDriver` | () - Retrieve information about the next available database driver. |
| No | `UseODBCDatabase` | () - Add ODBC support to the database commandset. |

## DatabasePostgreSQL

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UsePostgreSQLDatabase` | ([LibraryName$]) - Add PostgreSQL support to the database commandset. |

## DatabaseSQLite

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseSQLiteDatabase` | ([LibraryName$]) - Add SQLite support to the database commandset. |

## Date

10/14 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `AddDate` | (Date, Type, Value) - Return a new date. |
| No | `ConvertDate` | (Date, Format) - Convert a date between UTC and local time. |
| Yes | `Date` | ([Year, Month, Day, Hour, Minute, Second]) - Return the number of seconds elapsed since the 1st January 1970. |
| No | `DateUTC` | () - Return the number of seconds elapsed since the 1st January 1970 (UTC time). |
| Yes | `Day` | (Date) - Return the day value of the specified date (between 1 and 31). |
| Yes | `DayOfWeek` | (Date) - Return the day value in the week of the specified date (0=Sunday, 6=Saturday). |
| No | `DayOfYear` | (Date) - Return the number of days elapsed since the beginning of the year of the specified date (between 1 and 366). |
| Yes | `FormatDate` | (Mask$, Date) - Return a string representation of the Date, according to the specified Mask$. |
| Yes | `Hour` | (Date) - Return the hour value of the specified date (between 0 and 23). |
| Yes | `Minute` | (Date) - Return the minute value of the specified date (between 0 and 59). |
| Yes | `Month` | (Date) - Return the month value of the specified date (between 1 and 12). |
| No | `ParseDate` | (Mask$, Date$) - Transform a string date into a regular date value, to be used with other date functions. |
| Yes | `Second` | (Date) - Return the second value of the specified date (between 0 and 59). |
| Yes | `Year` | (Date) - Return the year value of the specified date. |

## DebuggerFunctions

0/18 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ClearDebugOutput` | () - Clear the content of the debug output window. |
| No | `CloseDebugOutput` | () - Close the debug output window. |
| No | `CopyDebugOutput` | () - Copy the content of the debug output window into the clipboard. |
| No | `DebuggerError` | (Message$) - Stop execution and show a debugger error. |
| No | `DebuggerWarning` | (Message$) - Show a debugger warning. |
| No | `PurifierGranularity` | (GlobalGranularity, LocalGranularity, StringGranularity, DynamicGranularity) - Change the Purifier check granularities. |
| No | `ResetProfiler` | () - Reset the line counts of the profiler. |
| No | `SaveDebugOutput` | (Filename$) - Save the content of the debug output window to a file. |
| No | `ShowAssemblyViewer` | () - Show the assembly viewer window. |
| No | `ShowCallstack` | () - Show the callstack window. |
| No | `ShowDebugOutput` | () - Open the debug output window. |
| No | `ShowLibraryViewer` | ([Library$ [, #Object]]) - Show the library viewer, optionally displaying an object from a library. |
| No | `ShowMemoryViewer` | ([*Buffer, Length]) - Show the memory viewer, with an optional memory buffer to display. |
| No | `ShowProfiler` | () - Show the profiler window. |
| No | `ShowVariableViewer` | () - Show the variable viewer window. |
| No | `ShowWatchlist` | () - Show the watchlist window. |
| No | `StartProfiler` | () - Start the profiler. |
| No | `StopProfiler` | () - Stop the profiler. |

## Desktop

0/16 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `DesktopDepth` | (#Desktop) - Return the depth (bits per pixels) of the specified desktop. |
| No | `DesktopFrequency` | (#Desktop) - Return the frequency (in Hz) of the specified desktop. |
| No | `DesktopHeight` | (#Desktop) - Return the height (in pixels) of the specified desktop. |
| No | `DesktopMouseX` | () - Return the absolute x mouse position. |
| No | `DesktopMouseY` | () - Return the absolute y mouse position. |
| No | `DesktopName` | (#Desktop) - Return the system name of the specified desktop, if any. |
| No | `DesktopResolutionX` | () - Return the desktop X resolution scale factor (1 if no scaling is applied). |
| No | `DesktopResolutionY` | () - Return the desktop Y resolution scale factor (1 if no scaling is applied). |
| No | `DesktopScaledX` | (x) - Scale a value on the X axis according to current desktop resolution. |
| No | `DesktopScaledY` | (y) - Scale a value on the Y axis according to current desktop resolution. |
| No | `DesktopUnscaledX` | (x) - Unscale a value on the X axis according to current desktop resolution. |
| No | `DesktopUnscaledY` | (y) - Unscale a value on the Y axis according to current desktop resolution. |
| No | `DesktopWidth` | (#Desktop) - Return the width (in pixels) of the specified desktop. |
| No | `DesktopX` | (#Desktop) - Return X coordinate of the upper left desktop corner. |
| No | `DesktopY` | (#Desktop) - Return Y coordinate of the upper left desktop corner. |
| No | `ExamineDesktops` | () - Retrieve information about user's desktops and return the number of desktops. |

## Dialog

0/10 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CreateDialog` | (#Dialog) - Create a new dialog. |
| No | `DialogError` | (#Dialog) - Return the error that occurred during the #Dialog creation. |
| No | `DialogGadget` | (#Dialog, Name$) - Return the specified #Gadget associated with the dialog. |
| No | `DialogID` | (#Dialog) - Return the unique ID which identifies the dialog in the operating system. |
| No | `DialogWindow` | (#Dialog) - Return the #Window associated with the dialog. |
| No | `FreeDialog` | (#Dialog) - Free the specified dialog. |
| No | `IsDialog` | (#Dialog) - Test if the given dialog is initialized. |
| No | `OpenXMLDialog` | (#Dialog, #XML, Name$ [, x, y [, Width, Height [, ParentID]]]) - Create a dialog from an XML object. |
| No | `RefreshDialog` | (#Dialog) - Recalculate the dialog dimension and adjust it if necessary. |
| No | `XmlDialogGadget` | (#Dialog, #XML, Name$ [, x, y [, Width, Height]]) - Create a dialog gadget (a ContainerGadget()) from an XML object. |

## DialogOpenGL

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseDialogOpenGLGadget` | () - Enable OpenGLGadget() support for the dialog library. |

## DialogScintilla

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseDialogScintillaGadget` | () - Enable ScintillaGadget() support for the dialog library. |

## DialogWeb

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseDialogWebGadget` | () - Enable WebGadget() support for the dialog library. |

## DialogWebView

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseDialogWebViewGadget` | () - Enable WebViewGadget() support for the dialog library. |

## DragDrop

0/22 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `DragFiles` | (Files$ [, Actions]) - Start a Drag and Drop operation with a list of Files. |
| No | `DraggedItemIndex` | () - Get the gadget dragged item index. |
| No | `DragImage` | (ImageID [, Actions]) - Start a Drag and Drop operation with the given Image. |
| No | `DragOSFormats` | (Formats(), Count [, Actions]) - Start a Drag and Drop operation with multiple clipboard formats. |
| No | `DragPrivate` | (Type [, Actions]) - Start a Drag and Drop operation within the current program only with the given private type. |
| No | `DragText` | (Text$ [, Actions]) - Start a Drag and Drop operation with the given Text. |
| No | `EnableGadgetDrop` | (#Gadget, Format, Actions [, PrivateType]) - Enable the Gadget as a Drag and Drop target for the given Format. |
| No | `EnableWindowDrop` | (#Window, Format, Actions [, PrivateType]) - Enable the Window as a Drag and Drop target for the given Format. |
| No | `EventDropAction` | () - Return the action that is to be taken with the dropped data. |
| No | `EventDropBuffer` | () - Return the raw memory buffer of the data received from a Drag and Drop event. |
| No | `EventDropFiles` | () - Return the Filenames received from a Drag and Drop event. |
| No | `EventDropImage` | (#Image [, Depth]) - Create a new Image from the data received from a Drag and Drop event. |
| No | `EventDropPrivate` | () - Return the type that was specified when starting a private Drag and Drop operation. |
| No | `EventDropSize` | () - Return the size of the buffer of data received from a Drag and Drop event. |
| No | `EventDropText` | () - Return the text received from a Drag and Drop event. |
| No | `EventDropType` | () - Return the type of format that was received from the Drag and Drop event. |
| No | `EventDropX` | () - Return the X position where the drop event took place in the gadget/window. |
| No | `EventDropY` | () - Return the Y position where the drop event took place in the gadget/window. |
| No | `ExamineDraggedItems` | () - Examine the dragged items from a multi-items gadget. |
| No | `NextDraggedItem` | () - Retrieve information about the next dragged item. |
| No | `SetDragCallback` | (@ProcedureName()) - Set a callback to provide feedback while dragging. |
| No | `SetDropCallback` | (@ProcedureName()) - Set a callback to provide feedback while dropping. |

## Engine3D

0/4 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `Add3DArchive` | (Filename$, Type) - Add a new archive as path to search. |
| No | `AntialiasingMode` | (Mode) - Set the fullscreen antialiasing mode for the 3D world. |
| No | `InitEngine3D` | ([Flags [, LibraryName$]]) - Initialise the 3D environment for later use. |
| No | `Parse3DScripts` | () - Parse all the scripts found in the declared path or archives. |

## Entity

0/59 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddSubEntity` | (#Entity, #SubEntity, Type [, OffsetX, OffsetY, OffsetZ [, Axis [, SizeX, SizeY, SizeZ]]]) - Add a sub entity to the #Entity. |
| No | `ApplyEntityForce` | (#Entity, x, y, z [, PositionX, PositionY, PositionZ [, Mode [, PositionMode]]]) - Apply a force to the #Entity. |
| No | `ApplyEntityImpulse` | (#Entity, x, y, z [, PositionX, PositionY, PositionZ [, Mode]]) - Apply a force to the #Entity. |
| No | `ApplyEntityTorque` | (#Entity, x, y, z [, Mode]) - Apply a rotation force to the #Entity. |
| No | `ApplyEntityTorqueImpulse` | (#Entity, x, y, z [, Mode]) - Apply a rotation force to the #Entity. |
| No | `AttachEntityObject` | (#Entity, Bone$, ObjectID [, x, y, z, Pitch, Yaw, Roll]) - Attach an object to the #Entity. |
| No | `CopyEntity` | (#Entity, #NewEntity) - Copy the specified #Entity on the #NewEntity. |
| No | `CreateEntity` | (#Entity [, MeshID, MaterialID [, x, y, z [, PickMask [, VisibilityMask]]]]) - Create a new #Entity based on the specified Mesh and Material at the given position. |
| No | `CreateEntityBody` | (#Entity, Type [, Mass [, Restitution, Friction [, Axis [, SizeX, SizeY, SizeZ]]]]) - Create a new body for the #Entity. |
| No | `DetachEntityObject` | (#Entity, ObjectID) - Detach the object from the #Entity. |
| No | `DisableEntityBody` | (#Entity, Disable) - Disable the #Entity body, so the physics are ignored. |
| No | `EnableManualEntityBoneControl` | (#Entity, Bone$, State, InheritOrientation) - Enable the #Entity bone manual control. |
| No | `EntityAngularFactor` | (#Entity, x, y, z) - Change the angular factor of the #Entity. |
| No | `EntityBonePitch` | (#Entity, Bone$) - Return the pitch of the bone, relative to the #Entity. |
| No | `EntityBoneRoll` | (#Entity, Bone$) - Return the roll of the bone, relative to the #Entity. |
| No | `EntityBoneX` | (#Entity, Bone$ [, OffsetX, OffsetY, OffsetZ]) - Return the x position of the bone, relative to the #Entity. |
| No | `EntityBoneY` | (#Entity, Bone$ [, OffsetX, OffsetY, OffsetZ]) - Return the y position of the bone, relative to the #Entity. |
| No | `EntityBoneYaw` | (#Entity, Bone$) - Return the yaw of the bone, relative to the #Entity. |
| No | `EntityBoneZ` | (#Entity, Bone$ [, OffsetX, OffsetY, OffsetZ]) - Return the z position of the bone, relative to the #Entity. |
| No | `EntityBoundingBox` | (#Entity, Flags) - Return the position of the #Entity bounding box. |
| No | `EntityCollide` | (#Entity, #Entity2) - Test if the entities are colliding. |
| No | `EntityCustomParameter` | (#Entity, SubEntityIndex, ParameterIndex, Value1, Value2, Value3, Value4) - Set a custom parameter to the #Entity shader script. |
| No | `EntityDirection` | (#Entity, x, y, z [, Mode, LocalDirectionVector]) - Change the direction of the #Entity. |
| No | `EntityDirectionX` | (#Entity) - Return the x direction factor of the #Entity. |
| No | `EntityDirectionY` | (#Entity) - Return the y direction factor of the #Entity. |
| No | `EntityDirectionZ` | (#Entity) - Return the z direction factor of the #Entity. |
| No | `EntityFixedYawAxis` | (#Entity, Mode [, VectorX, VectorY, VectorZ]) - Change the way the yaw axis is managed. |
| No | `EntityID` | (#Entity) - Return the unique #Entity system identifier. |
| No | `EntityLinearFactor` | (#Entity, x, y, z) - Change the linear factor of the #Entity. |
| No | `EntityLookAt` | (#Entity, x, y, z [, DirectionX, DirectionY, DirectionZ]) - Change the #Entity orientation to look at the specified point. |
| No | `EntityMesh` | (#Entity, MeshID) - Change the #Entity mesh with the new one. |
| No | `EntityParentNode` | (#Entity) - Return the parent #Entity node. |
| No | `EntityPitch` | (#Entity [, Mode]) - Return the pitch of the #Entity. |
| No | `EntityRenderMode` | (#Entity, Mode) - Change the way the #Entity will be rendered. |
| No | `EntityRoll` | (#Entity [, Mode]) - Return the roll of the #Entity. |
| No | `EntityVelocity` | (#Entity, x, y, z) - Change the linear velocity of the #Entity. |
| No | `EntityX` | (#Entity [, Mode]) - Return the x position of the #Entity. |
| No | `EntityY` | (#Entity [, Mode]) - Return the y position of the #Entity. |
| No | `EntityYaw` | (#Entity [, Mode]) - Return the yaw of the #Entity. |
| No | `EntityZ` | (#Entity [, Mode]) - Return the z position of the #Entity. |
| No | `FetchEntityMaterial` | (#Entity, #Material, [, SubEntity]) - Fetch the #Entity material. |
| No | `FreeEntity` | (#Entity) - Free the specified #Entity. |
| No | `FreeEntityBody` | (#Entity) - Free the body associated with the #Entity. |
| No | `FreeEntityJoints` | (#Entity) - Free all joints associated with the #Entity. |
| No | `GetEntityAttribute` | (#Entity, Attribute) - Get the #Entity attribute value. |
| No | `GetEntityCollisionGroup` | (#Entity) - Get the entity collision group. |
| No | `GetEntityCollisionMask` | (#Entity) - Get the entity collision mask. |
| No | `GetEntityMesh` | (#Entity) - Return the #Mesh associated with the #Entity. |
| No | `HideEntity` | (#Entity, Hide) - Hide or show the specified #Entity. |
| No | `IsEntity` | (#Entity) - Test if the given '#Entity' is an initialized entity object. |
| No | `MoveEntity` | (#Entity, x, y, z [, Mode]) - Move the #Entity relative to its previous position. |
| No | `MoveEntityBone` | (#Entity, Bone$, x, y, z , Mode) - Move the #Entity bone relative to its previous position. |
| No | `RotateEntity` | (#Entity, x, y, z [, Mode]) - Rotate the #Entity. |
| No | `RotateEntityBone` | (#Entity, Bone$, x, y, z , Mode) - Rotate the #Entity bone. |
| No | `ScaleEntity` | (#Entity, x, y, z [, Mode]) - Scale the #Entity. |
| No | `ScaleEntityBone` | (#Entity, Bone$, x, y, z , Mode) - Scale the #Entity bone. |
| No | `SetEntityAttribute` | (#Entity, Attribute, Value) - Set an #Entity attribute value. |
| No | `SetEntityCollisionFilter` | (#Entity, Group, Mask) - Set the entity collision group and mask. |
| No | `SetEntityMaterial` | (#Entity, MaterialID [, SubEntity]) - Set the #Entity material. |

## EntityAnimation

0/12 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddEntityAnimationTime` | (#Entity, Animation$, Time) - Add time to the specified #Entity animation. |
| No | `EntityAnimationBlendMode` | (#Entity, Mode) - Change the #Entity blend mode. |
| No | `EntityAnimationStatus` | (#Entity, Animation$) - Return the specified #Entity animation status. |
| No | `GetEntityAnimationLength` | (#Entity, Animation$) - Get the #Entity animation length. |
| No | `GetEntityAnimationTime` | (#Entity, Animation$) - Get the current #Entity animation time. |
| No | `GetEntityAnimationWeight` | (#Entity, Animation$) - Get the #Entity animation weight. |
| No | `SetEntityAnimationLength` | (#Entity, Animation$, Length) - Set the #Entity animation length. |
| No | `SetEntityAnimationTime` | (#Entity, Animation$, Time) - Set the current #Entity animation time. |
| No | `SetEntityAnimationWeight` | (#Entity, Animation$, Weight) - Set the #Entity animation weight. |
| No | `StartEntityAnimation` | (#Entity, Animation$ [, Flags]) - Start the specified #Entity animation. |
| No | `StopEntityAnimation` | (#Entity, Animation$) - Stop the specified #Entity animation. |
| No | `UpdateEntityAnimation` | (#Entity, Animation$) - Update the specified #Entity animation. |

## Event

4/10 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `BindEvent` | (Event, @Callback() [, Window [, Object [, EventType]]]) - Bind an event to a callback. |
| No | `Event` | () - Return the current event. |
| No | `EventData` | () - Return the data associated to the event. |
| Yes | `EventGadget` | () - Return the gadget number (#Gadget) which triggered the event. |
| Yes | `EventMenu` | () - Return the menu item (#MenuItem) which triggered the event. |
| No | `EventTimer` | () - Return the timer identifier which triggered the event. |
| Yes | `EventType` | () - After a WindowEvent() or WaitWindowEvent() command, use this function to determine which type the event is. |
| Yes | `EventWindow` | () - Return on which window (#Window) the event occurred. |
| No | `PostEvent` | (Event [, Window, Object [, Type [, Data]]]) - Post an event to the event queue. |
| No | `UnbindEvent` | (Event, @Callback() [, Window [, Object [, EventType]]]) - Unbind an event callback. |

## File

11/40 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `CloseFile` | (#File) - Close the specified file. |
| Yes | `CreateFile` | (#File, Filename$ [, Flags]) - Create an empty file. |
| Yes | `Eof` | (#File) - Return not null if the end of file has been reached, 0 otherwise. |
| No | `FileBuffersSize` | (#File, Size) - Change the size of the buffers for the specified file. |
| No | `FileID` | (#File) - Return the unique ID which identifies the file in the operating system. |
| Yes | `FileSeek` | (#File, NewPosition [, Mode]) - Change the read/write pointer position of the specified file. |
| No | `FlushFileBuffers` | (#File) - Force the write of the internal file buffer to the disk. |
| No | `IsFile` | (#File) - Test if the given '#File' is an initialized file object. |
| Yes | `Loc` | (#File) - Return the actual pointer position of the specified file. |
| Yes | `Lof` | (#File) - Return the length of the specified file. |
| Yes | `OpenFile` | (#File, Filename$ [, Flags]) - Open the designated file or create it if it doesn't exist. |
| No | `ReadAsciiCharacter` | (#File) - Read one ascii character from the specified file. |
| No | `ReadByte` | (#File) - Read one byte from the specified file. |
| No | `ReadCharacter` | (#File [, Format]) - Read a character from the specified file. |
| No | `ReadData` | (#File, *Buffer, Length) - Read content from the file in the buffer. |
| No | `ReadDouble` | (#File) - Read a double (8 bytes) from the specified file. |
| Yes | `ReadFile` | (#File, Filename$ [, Flags]) - Open an existing file for read only operations. |
| No | `ReadFloat` | (#File) - Read a float (4 bytes) from the specified file. |
| No | `ReadInteger` | (#File) - Read an integer from the specified file. |
| No | `ReadLong` | (#File) - Read a long (4 bytes) from the specified file. |
| No | `ReadQuad` | (#File) - Read a quad (8 bytes) from the specified file. |
| Yes | `ReadString` | (#File [, Flags [, Length]]) - Read a string from the specified file. |
| No | `ReadStringFormat` | (#File) - Try to detect the string format in the file from a BOM (Byte Order Mark). |
| No | `ReadUnicodeCharacter` | (#File) - Read one unicode character from the specified file. |
| No | `ReadWord` | (#File) - Read a word (2 bytes) from the specified file. |
| No | `TruncateFile` | (#File) - End the file at the current position and cut all following data. |
| No | `WriteAsciiCharacter` | (#File, Number) - Write an ascii character to the specified file. |
| No | `WriteByte` | (#File, Number) - Write a byte sized number to the specified file. |
| No | `WriteCharacter` | (#File, Number [, Format]) - Write a character to the specified file. |
| No | `WriteData` | (#File, *Buffer, Length) - Write the content of the specified memory buffer to the specified file. |
| No | `WriteDouble` | (#File, Number) - Write a double number to the specified file. |
| No | `WriteFloat` | (#File, Number) - Write a float number to the specified file. |
| No | `WriteInteger` | (#File, Number) - Write an integer to the specified file. |
| No | `WriteLong` | (#File, Number) - Write a long number to the specified file. |
| No | `WriteQuad` | (#File, Number) - Write a quad number to the specified file. |
| Yes | `WriteString` | (#File, Text$ [, Format]) - Write a string to the specified file. |
| No | `WriteStringFormat` | (#File, Format) - Write a BOM (Byte Order Mark) to identify the string format used in the file. |
| Yes | `WriteStringN` | (#File, Text$ [, Format]) - Write a string to the specified file and add the 'end of line' character. |
| No | `WriteUnicodeCharacter` | (#File, Number) - Write a unicode character to the specified file. |
| No | `WriteWord` | (#File, Number) - Write a word number to the specified file. |

## FileSystem

3/29 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CheckFilename` | (Filename$) - Check if the filename is valid. |
| No | `CopyDirectory` | (SourceDirectory$, DestinationDirectory$, Pattern$ [, Flags]) - Copy the source directory to the destination. |
| No | `CopyFile` | (SourceFilename$, DestinationFilename$) - Copy the source file to the destination. |
| No | `CreateDirectory` | (DirectoryName$) - Create a new directory. |
| No | `DeleteDirectory` | (Directory$, Pattern$ [, Flags]) - Delete the directory. |
| Yes | `DeleteFile` | (Filename$ [, Flags]) - Delete the specified file. |
| No | `DirectoryEntryAttributes` | (#Directory) - Return the attributes of the current entry in the directory. |
| No | `DirectoryEntryDate` | (#Directory, Type) - Return the modified/created/accessed date of the current entry in the directory. |
| No | `DirectoryEntryName` | (#Directory) - Return the name of the current entry in the directory. |
| No | `DirectoryEntrySize` | (#Directory) - Return the size of the current entry in the directory. |
| No | `DirectoryEntryType` | (#Directory) - Return the type of the current entry in the directory. |
| No | `ExamineDirectory` | (#Directory, DirectoryName$, Pattern$) - Start to examine the specified directory for future listing. |
| Yes | `FileSize` | (Filename$) - Return the size of the specified file. |
| No | `FinishDirectory` | (#Directory) - Finish examining the directory. |
| No | `GetCurrentDirectory` | () - Return the path of the current directory. |
| No | `GetExtensionPart` | (FullPathName$) - Retrieve the extension part of a full path. |
| No | `GetFileAttributes` | (Filename$) - Return the attributes of the specified file. |
| No | `GetFileDate` | (Filename$, DateType) - Return the modified/created/accessed date of the specified file. |
| No | `GetFilePart` | (FullPathName$ [, Flags]) - Retrieve the file part of a full path. |
| No | `GetHomeDirectory` | () - Return the full path of the currently logged user directory. |
| No | `GetPathPart` | (FullPathName$) - Retrieve the path part of a full path. |
| No | `GetTemporaryDirectory` | () - Return the full path of the temporary directory. |
| No | `GetUserDirectory` | (DirectoryType) - Return the path of the specified user directory. |
| No | `IsDirectory` | (#Directory) - Test if the given '#Directory' is an initialized directory object. |
| No | `NextDirectoryEntry` | (#Directory) - Go to the next directory entry. |
| Yes | `RenameFile` | (OldFilename$, NewFilename$) - Rename the old file to the new file. |
| No | `SetCurrentDirectory` | (Path$) - Change the path of the current directory to the specified one. |
| No | `SetFileAttributes` | (Filename$, Attributes) - Change the attributes of the specified file. |
| No | `SetFileDate` | (Filename$, Type, Date) - Change the modified/created/accessed date of the specified file. |

## Font

0/5 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `FontID` | (#Font) - Return the unique ID which identifies the font in the operating system. |
| No | `FreeFont` | (#Font) - Free the given Font. |
| No | `IsFont` | (#Font) - Test if the given '#Font' is an initialized font object. |
| No | `LoadFont` | (#Font, Name$, Size [,Flags]) - Try to open the specified font. |
| No | `RegisterFontFile` | (FileName$) - Register a font file for use by the application. |

## Ftp

0/23 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AbortFTPFile` | (#Ftp) - Abort the current asynchronous file operation. |
| No | `CheckFTPConnection` | (#Ftp) - Check if the FTP server is still responding. |
| No | `CloseFTP` | (#Ftp) - Close the specified client connection and send a notification to the server. |
| No | `CreateFTPDirectory` | (#Ftp, Directory$) - Create a new directory on the FTP server. |
| No | `DeleteFTPDirectory` | (#Ftp, Directory$) - Delete a directory on the FTP server. |
| No | `DeleteFTPFile` | (#Ftp, Filename$) - Delete a file on the FTP server. |
| No | `ExamineFTPDirectory` | (#Ftp) - Examine the current directory on the FTP server. |
| No | `FinishFTPDirectory` | (#Ftp) - Free the data allocated with ExamineFTPDirectory. |
| No | `FTPDirectoryEntryAttributes` | (#Ftp) - Return the attributes of the current entry in the last FTP listing. |
| No | `FTPDirectoryEntryDate` | (#Ftp) - Return the modified date of the current entry in the last FTP listing. |
| No | `FTPDirectoryEntryName` | (#Ftp) - Return the name of the current entry in the last FTP listing. |
| No | `FTPDirectoryEntryRaw` | (#Ftp) - Return the raw line of the current entry in the last FTP listing. |
| No | `FTPDirectoryEntrySize` | (#Ftp) - Return the size of the current entry in the last FTP listing. |
| No | `FTPDirectoryEntryType` | (#Ftp) - Return the type of the current entry in the last FTP listing. |
| No | `FTPProgress` | (#Ftp) - Return the progress of the current file transfer. |
| No | `GetFTPDirectory` | (#Ftp) - Return the current directory. |
| No | `IsFTP` | (#Ftp) - Test if the given '#Ftp' is an initialized FTP object. |
| No | `NextFTPDirectoryEntry` | (#Ftp) - Go to the next FTP directory entry. |
| No | `OpenFTP` | (#Ftp, ServerName$, User$, Password$ [, Flags [, Port]]) - Try to open a connection on the specified server. |
| No | `ReceiveFTPFile` | (#Ftp, RemoteFilename$, Filename$ [, Asynchronous]) - Receive a file from a FTP Server. |
| No | `RenameFTPFile` | (#Ftp, Filename$, NewFilename$) - Rename or move a file on the FTP server. |
| No | `SendFTPFile` | (#Ftp, Filename$, RemoteFilename$ [, Asynchronous]) - Send a file to a FTP Server. |
| No | `SetFTPDirectory` | (#Ftp, Directory$) - Change the current directory. |

## Gadget

68/82 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `AddGadgetColumn` | (#Gadget, Position, Title$, Width) - Add a column to the specified #Gadget. |
| Yes | `AddGadgetItem` | (#Gadget, Position, Text$ [, ImageID [, Flags]]) - Add an item to a gadget (ListView, ComboBox etc.). |
| No | `BindGadgetEvent` | (#Gadget, @Callback() [, EventType]) - Bind a gadget event to a callback. |
| Yes | `ButtonGadget` | (#Gadget, x, y, Width, Height, Text$ [, Flags]) - Create a button gadget in the current GadgetList. |
| Yes | `ButtonImageGadget` | (#Gadget, x, y, Width, Height, ImageID [, Flags]) - Create a button gadget in the current GadgetList. |
| Yes | `CalendarGadget` | (#Gadget, x, y, Width, Height [, Date [, Flags]]) - Create a Date selection Calendar gadget in the current GadgetList. |
| Yes | `CanvasGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a canvas gadget in the current GadgetList. |
| Yes | `CanvasOutput` | (#Gadget) - Return the OutputID of the specified #Gadget to perform 2D rendering operation on it. |
| No | `CanvasVectorOutput` | (#Gadget [, Unit]) - Return the OutputID of the specified #Gadget to perform vector drawing on it. |
| Yes | `CheckBoxGadget` | (#Gadget, x, y, Width, Height, Text$ [, Flags]) - Create a checkbox gadget in the GadgetList. |
| Yes | `ClearGadgetItems` | (#Gadget) - Clear all the items of the specified #Gadget. |
| Yes | `CloseGadgetList` | () - Close the current GadgetList creation and go back to the previous one. |
| Yes | `ComboBoxGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a combobox gadget in the current GadgetList. |
| Yes | `ContainerGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a gadget container in the current GadgetList. |
| Yes | `CountGadgetItems` | (#Gadget) - Count all the items of the specified #Gadget. |
| Yes | `DateGadget` | (#Gadget, x, y, Width, Height [, Mask$ [, Date [, Flags]]]) - Create a Date/Time selector Gadget. |
| Yes | `DisableGadget` | (#Gadget, State) - Disable or enable a gadget. |
| No | `DisableGadgetRedraw` | (#Gadget, State) - Disable or enable the redrawing of a gadget to speed up mass updates. |
| Yes | `EditorGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create an Editor gadget in the current GadgetList. |
| Yes | `ExplorerComboGadget` | (#Gadget, x, y, Width, Height, Directory$ [, Flags]) - Create a File/Directory Combo Gadget. |
| Yes | `ExplorerListGadget` | (#Gadget, x, y, Width, Height, Directory$ [, Flags]) - Create a File/Directory List Gadget. |
| Yes | `ExplorerTreeGadget` | (#Gadget, x, y, Width, Height, Directory$ [, Flags]) - Create a File/Directory Tree Gadget. |
| Yes | `FrameGadget` | (#Gadget, x, y, Width, Height, Text$ [, Flags]) - Create a frame gadget in the current GadgetList. |
| Yes | `FreeGadget` | (#Gadget) - Free (remove) the #Gadget. |
| Yes | `GadgetHeight` | (#Gadget [, Mode]) - Return the height of the #Gadget, in pixels. |
| Yes | `GadgetID` | (#Gadget) - Return the ID of the #Gadget. |
| No | `GadgetItemID` | (#Gadget, Item) - Return the system ID of the specified gadget item. |
| No | `GadgetToolTip` | (#Gadget, Text$) - Associate the specified Text$ with the #Gadget. |
| Yes | `GadgetType` | (#Gadget) - Return the type of the specified #Gadget. |
| Yes | `GadgetWidth` | (#Gadget [, Mode]) - Return the width of the #Gadget, in pixels. |
| Yes | `GadgetX` | (#Gadget [, Mode]) - Return the X position of the #Gadget, in pixels. |
| Yes | `GadgetY` | (#Gadget [, Mode]) - Return the Y position of the #Gadget, in pixels. |
| Yes | `GetActiveGadget` | () - Get the Gadget that has the focus. |
| Yes | `GetGadgetAttribute` | (#Gadget, Attribute) - Return the specified #Gadget attribute value. |
| No | `GetGadgetColor` | (#Gadget, ColorType) - Get the currently set color of the Gadget. |
| Yes | `GetGadgetData` | (#Gadget) - Return the value previously stored for this Gadget. |
| No | `GetGadgetFont` | (#Gadget) - Get the FontID associated with the specified gadget. |
| Yes | `GetGadgetItemAttribute` | (#Gadget, Item, Attribute [, Column]) - Return the specified #Gadget item attribute value. |
| No | `GetGadgetItemColor` | (#Gadget, Item, ColorType [, Column]) - Get the currently set color of the Gadget item. |
| Yes | `GetGadgetItemData` | (#Gadget, Item) - Return the value previously stored with this gadget item. |
| Yes | `GetGadgetItemState` | (#Gadget, Position) - Return the #Gadget item state. |
| Yes | `GetGadgetItemText` | (#Gadget, Position [, Column]) - Return the text content of the specified #Gadget item. |
| Yes | `GetGadgetState` | (#Gadget) - Return the gadget current state. |
| Yes | `GetGadgetText` | (#Gadget) - Return the gadget text content of the specified #Gadget. |
| Yes | `HideGadget` | (#Gadget, State) - Hide or show a gadget. |
| Yes | `HyperLinkGadget` | (#Gadget, x, y, Width, Height, Text$, Color [, Flags]) - Create a HyperLink gadget in the current GadgetList. |
| Yes | `ImageGadget` | (#Gadget, x, y, Width, Height, ImageID [, Flags]) - Create an Image gadget in the current GadgetList. |
| Yes | `IPAddressGadget` | (#Gadget, x, y, Width, Height) - Create an IP Address gadget in the current GadgetList. |
| Yes | `IsGadget` | (#Gadget) - Test if the given '#Gadget' is an initialized gadget object. |
| Yes | `ListIconGadget` | (#Gadget, x, y, Width, Height, FirstColumnTitle$, FirstColumnWidth [, Flags]) - Create a ListIcon gadget in the current GadgetList. |
| Yes | `ListViewGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a ListView gadget in the current GadgetList. |
| Yes | `OpenGadgetList` | (#Gadget [, Item]) - Make the Gadget the current gadget list. |
| Yes | `OptionGadget` | (#Gadget, x, y, Width, Height, Text$) - Create an Option gadget in the current GadgetList. |
| Yes | `PanelGadget` | (#Gadget, x, y, Width, Height) - Create a Panel gadget in the current GadgetList. |
| Yes | `ProgressBarGadget` | (#Gadget, x, y, Width, Height, Minimum, Maximum [, Flags]) - Create a ProgressBar gadget in the current GadgetList. |
| Yes | `RemoveGadgetColumn` | (#Gadget, Column) - Remove a column from the specified #Gadget. |
| Yes | `RemoveGadgetItem` | (#Gadget, Position) - Remove the item at the specified position from the #Gadget. |
| Yes | `ResizeGadget` | (#Gadget, x, y, Width, Height) - Move and resize the specified #Gadget to the given position and dimensions. |
| Yes | `ScrollAreaGadget` | (#Gadget, x, y, Width, Height, ScrollAreaWidth, ScrollAreaHeight [, ScrollStep [, Flags]]) - Create a ScrollArea gadget in the current GadgetList. |
| Yes | `ScrollBarGadget` | (#Gadget, x, y, Width, Height, Min, Max, PageLength [, Flags]) - Create a ScrollBar gadget in the current GadgetList. |
| Yes | `SetActiveGadget` | (#Gadget) - Set the focus on the specified Gadget. |
| Yes | `SetGadgetAttribute` | (#Gadget, Attribute, Value) - Change the specified #Gadget attribute. |
| No | `SetGadgetColor` | (#Gadget, ColorType, Color) - Change a color in the Gadget. |
| Yes | `SetGadgetData` | (#Gadget, Value) - Store a value for the Gadget. |
| No | `SetGadgetFont` | (#Gadget, FontID()) - Change the font which will be used by newly created gadgets. |
| Yes | `SetGadgetItemAttribute` | (#Gadget, Item, Attribute, Value [, Column]) - Change the specified #Gadget item attribute. |
| No | `SetGadgetItemColor` | (#Gadget, Item, ColorType, Color [, Column]) - Change a color in the Gadget item. |
| Yes | `SetGadgetItemData` | (#Gadget, Item, Value) - Store a Long value with this Gadget item. |
| No | `SetGadgetItemImage` | (#Gadget, Item, ImageID) - Change the image of the specified gadget item. |
| Yes | `SetGadgetItemState` | (#Gadget, Position, State) - Change the #Gadget item state. |
| Yes | `SetGadgetItemText` | (#Gadget, Position, Text$ [, Column]) - Change the #Gadget item text content. |
| Yes | `SetGadgetState` | (#Gadget, State) - Change the state of the #Gadget. |
| Yes | `SetGadgetText` | (#Gadget, Text$) - Change the gadget text content of the specified #Gadget. |
| Yes | `ShortcutGadget` | (#Gadget, x, y, Width, Height, Shortcut) - Create a Gadget to select a keyboard shortcut in the current GadgetList. |
| Yes | `SpinGadget` | (#Gadget, x, y, Width, Height, Minimum, Maximum [, Flags]) - Create a Spin gadget in the current GadgetList. |
| Yes | `SplitterGadget` | (#Gadget, x, y, Width, Height, #Gadget1, #Gadget2 [, Flags]) - Create a Splitter gadget in the current GadgetList. |
| Yes | `StringGadget` | (#Gadget, x, y, Width, Height, Content$ [, Flags]) - Create a String gadget in the current GadgetList. |
| Yes | `TextGadget` | (#Gadget, x, y, Width, Height, Text$ [, Flags]) - Create a Text gadget in the current GadgetList. |
| Yes | `TrackBarGadget` | (#Gadget, x, y, Width, Height, Minimum, Maximum [, Flags]) - Create a TrackBar gadget in the current GadgetList. |
| Yes | `TreeGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a Tree gadget in the current GadgetList. |
| No | `UnbindGadgetEvent` | (#Gadget, @Callback() [, EventType]) - Unbind a gadget event callback. |
| No | `UseGadgetList` | (WindowID) - Make the specified WindowID the current gadget list to add gadgets on it. |

## GadgetOpenGL

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `OpenGLGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create an OpenGL gadget in the current GadgetList. |

## GadgetWeb

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `WebGadget` | (#Gadget, x, y, Width, Height, URL$ [, Flags]) - Create a Web gadget in the current GadgetList. |

## Help

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CloseHelp` | () - Close a previously opened Help window. |
| No | `OpenHelp` | (Filename$, Topic$) - Open a new Help window. |

## HID

0/17 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CloseHID` | (#HID) - Close the specified HID. |
| No | `DisableHIDBlockingMode` | (#HID, Status). |
| No | `ExamineHIDs` | ([VendorId [, ProductId]]) - Start the enumeration of available HIDs. |
| No | `GetHIDIndexedString` | (#HID, Index) - Get the specified indexed string value. |
| No | `GetHIDReportDescriptor` | (#HID) - Get the HID report descriptor. |
| No | `HIDError` | () - Read the last error information. |
| No | `HIDInfo` | (Type [, #HID]) - Get more information about the HID. |
| No | `IsHID` | (#HID) - Check if the given '#HID' is an initialized HID object. |
| No | `NextHID` | () - Get the next HID in the enumeration started with ExamineHIDs(). |
| No | `OpenHID` | (#HID, VendorId, ProductId [, SerialNumber]) - Open the specified HID for read or write operations. |
| No | `OpenHIDPath` | (#HID, Path$) - Open the specified HID for read or write operations using a path. |
| No | `ReadHIDData` | (#HID, *Buffer, Size [, Timeout]) - Read data from the specified HID. |
| No | `ReceiveHIDFeatureReport` | (#HID, *Buffer, Size) - Receive a feature report. |
| No | `ReceiveHIDInputReport` | (#HID, *Buffer, Size) - Receive an input report. |
| No | `SendHIDFeatureReport` | (#HID, *Buffer, Size) - Send a feature report. |
| No | `SendHIDOutputReport` | (#HID, *Buffer, Size) - Send an output report. |
| No | `WriteHIDData` | (#HID, *Buffer, Size) - Write data to the specified HID. |

## Http

0/15 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AbortHTTP` | (HttpConnection) - Abort the download of the specified HTTP connection. |
| No | `FinishHTTP` | (HttpConnection) - Release the resources of the specified HTTP connection. |
| No | `GetURLPart` | (URL$, Parameter$) - Get the specific part of a URL. |
| No | `HTTPInfo` | (HttpRequest, Type [, Flags]) - Get info from an HTTP request. |
| No | `HTTPMemory` | (HttpRequest) - Return the full memory buffer of the HTTP request. |
| No | `HTTPProgress` | (HttpConnection) - Get the progress on the specified HTTP connection. |
| No | `HTTPProxy` | (URL$ [, User$, Password$]) - Set a proxy to use for HTTP commands. |
| No | `HTTPRequest` | (Type, URL$ [, Data$ [, Flags [, Headers()]]]) - Execute an HTTP request. |
| No | `HTTPRequestMemory` | (Type, URL$ [, *Data, DataSize [, Flags [, Headers()]]]) - Execute an HTTP request. |
| No | `HTTPTimeout` | (ConnectTimeout [, GlobalTimeout]) - Set custom timeout (in milliseconds) for HTTP commands. |
| No | `ReceiveHTTPFile` | (URL$, Filename$ [, Flags [, UserAgent$]]) - Download a file from a URL. |
| No | `ReceiveHTTPMemory` | (URL$ [, Flags [, UserAgent$]]) - Download a file from a URL to a memory buffer. |
| No | `SetURLPart` | (URL$, Parameter$, Value$) - Set the specific part of a URL. |
| No | `URLDecoder` | (URL$ [, Format]) - Return the decoded URL$. |
| No | `URLEncoder` | (URL$ [, Format]) - Return the URL$ encoded to HTTP format. |

## Image

7/24 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddImageFrame` | (#Image [, Index]) - Add a new frame to the image and set it as current frame. |
| No | `CatchImage` | (#Image, *Memory [, Size]) - Create a new #Image found at the specified memory address. |
| No | `CopyImage` | (#Image1, #Image2) - Create a new #Image2 identical to the source #Image1. |
| Yes | `CreateImage` | (#Image, Width, Height [, Depth [, BackColor]]) - Create an empty #Image which can be used to do rendering on it. |
| No | `EncodeImage` | (#Image [, ImagePlugin [, Flags [, Depth]]]) - Encode the specified image into memory. |
| Yes | `FreeImage` | (#Image) - Free the image and release its associated memory. |
| No | `GetImageFrame` | (#Image) - Get the current image frame. |
| No | `GetImageFrameDelay` | (#Image) - Return the current frame delay in milliseconds. |
| No | `GrabImage` | (#Image1, #Image2, x, y, Width, Height) - Create a new #Image2 with the selected area on the source #Image1. |
| No | `ImageDepth` | (#Image [, Flags]) - Return the depth of the image. |
| No | `ImageFormat` | (#Image) - Return the original format of the image. |
| No | `ImageFrameCount` | (#Image) - Return the number of frames in the image. |
| Yes | `ImageHeight` | (#Image) - Return the height, in pixel, of the image. |
| Yes | `ImageID` | (#Image) - Return the ImageID of the image. |
| No | `ImageOutput` | (#Image) - Return the OutputID of the image to perform 2D rendering operation on it. |
| No | `ImageVectorOutput` | (#Image [, Unit]) - Prepare the image for 2D vector graphic operations. |
| Yes | `ImageWidth` | (#Image) - Return the width, in pixel, of the image. |
| Yes | `IsImage` | (#Image) - Test if the given '#Image' is an initialized image object. |
| Yes | `LoadImage` | (#Image, Filename$ [, Flags]) - Load the specified image. |
| No | `RemoveImageFrame` | (#Image, Index) - Remove a frame from the image. |
| No | `ResizeImage` | (#Image, Width, Height [, Mode]) - Resize the image to the given dimension. |
| No | `SaveImage` | (#Image, Filename$ [, ImagePlugin [, Flags [, Depth]]]) - Save the specified image. |
| No | `SetImageFrame` | (#Image, Frame) - Change the current image frame. |
| No | `SetImageFrameDelay` | (#Image, Delay) - Set the current frame delay in milliseconds. |

## ImagePluginGIF

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseGIFImageDecoder` | () - Enable GIF support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |

## ImagePluginJPEG

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseJPEGImageDecoder` | () - Enable the JPEG support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |
| No | `UseJPEGImageEncoder` | () - Enable the JPEG support for SaveImage() and SaveSprite(). |

## ImagePluginJPEG2000

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseJPEG2000ImageDecoder` | () - Enable the JPEG 2000 support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |
| No | `UseJPEG2000ImageEncoder` | () - Enable the JPEG 2000 support for SaveImage() and SaveSprite(). |

## ImagePluginOS

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseOSImageDecoder` | () - Enable loading of every image format supported by the operating system for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |
| No | `UseOSImageEncoder` | () - Enable saving through the operating system codecs for SaveImage() and SaveSprite() (JPEG, PNG, BMP, TIFF and GIF). |

## ImagePluginPNG

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UsePNGImageDecoder` | () - Enable the PNG support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |
| No | `UsePNGImageEncoder` | () - Enable the PNG support for SaveImage() and SaveSprite(). |

## ImagePluginTGA

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseTGAImageDecoder` | () - Enable the TGA support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |

## ImagePluginTIFF

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseTIFFImageDecoder` | () - Enable the TIFF support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |

## ImagePluginWebP

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseWebPImageDecoder` | () - Enable the WebP support for CatchImage(), LoadImage(), CatchSprite() and LoadSprite(). |
| No | `UseWebPImageEncoder` | () - Enable the WebP support for SaveImage() and SaveSprite(). |

## Joint

0/11 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ConeTwistJoint` | (#Joint, EntityID, FxA, FyA, FzA [, EntityID2, FxB, FyB, FzB]) - Create a cone twist joint. |
| No | `EnableHingeJointAngularMotor` | (#Joint, Enable, TargetVelocity, MaxMotorImpulse) - Enable the angular motor on the hinge #Joint. |
| No | `FreeJoint` | (#Joint) - Free the specified #Joint. |
| No | `GenericJoint` | (#Joint, EntityID, FxA, FyA, FzA [, EntityID2, FxB, FyB, FzB]) - Create a generic joint. |
| No | `GetJointAttribute` | (#Joint, Attribute) - Return the specified #Joint attribute value. |
| No | `HingeJoint` | (#Joint, EntityID, PivotX, PivotY, PivotZ, AxisX, AxisY, AxisZ [, EntityID2, PivotX2, PivotY2, PivotZ2, AxisX2, AxisY2, AxisZ2]) - Create a hinge joint. |
| No | `HingeJointMotorTarget` | (#Joint, Angle, Velocity) - Set the motor target on the hinge #Joint. |
| No | `IsJoint` | (#Joint) - Test if the given '#Joint' is an initialized joint object. |
| No | `PointJoint` | (#Joint, EntityID, PivotX, PivotY, PivotZ [, EntityID2, PivotX2, PivotY2, PivotZ2]) - Create a point joint. |
| No | `SetJointAttribute` | (#Joint, Attribute, Value [, Index]) - Set the specified #Joint attribute value. |
| No | `SliderJoint` | (#Joint, EntityID, FxA, FyA, FzA [, EntityID2, FxB, FyB, FzB]) - Create a slider joint. |

## Joystick

0/7 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ExamineJoystick` | (#Joystick) - Examine the current state of the #Joystick. |
| No | `InitJoystick` | () - Initialize the joystick environment for later use. |
| No | `JoystickAxisX` | (#Joystick [, Pad [, Mode]]) - Return the X axis state. |
| No | `JoystickAxisY` | (#Joystick [, Pad [, Mode]]) - Return the Y axis state. |
| No | `JoystickAxisZ` | (#Joystick [, Pad [, Mode]]) - Return the Z axis state. |
| No | `JoystickButton` | (#Joystick, ButtonNumber) - Return 1 if the specified button is pressed. |
| No | `JoystickName` | (#Joystick) - Return the joystick name. |

## Json

0/53 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddJSONElement` | (*JSONValue [, Index]) - Add an element to a JSON array. |
| No | `AddJSONMember` | (*JSONValue, Key$) - Add a member to a JSON object. |
| No | `CatchJSON` | (#JSON, *Buffer, Size [, Flags]) - Parse JSON from a memory buffer. |
| No | `ClearJSONElements` | (*JSONValue) - Clear the elements of a JSON array. |
| No | `ClearJSONMembers` | (*JSONValue) - Clear the members of a JSON object. |
| No | `ComposeJSON` | (#JSON [, Flags]) - Compose JSON data into a string. |
| No | `CreateJSON` | (#JSON [, Flags]) - Create new JSON data with a value of type #PB_JSON_Null. |
| No | `ExamineJSONMembers` | (*JSONValue) - Examine the members of a JSON object. |
| No | `ExportJSON` | (#JSON, *Buffer, Size [, Flags]) - Export JSON data to a memory buffer. |
| No | `ExportJSONSize` | (#JSON [, Flags]) - Get the size that JSON data needs for the ExportJSON() function. |
| No | `ExtractJSONArray` | (*JSONValue, Array()) - Extract a JSON value into the specified Array(). |
| No | `ExtractJSONList` | (*JSONValue, List()) - Extract a JSON value into the specified List(). |
| No | `ExtractJSONMap` | (*JSONValue, Map()) - Extract a JSON value into the specified Map(). |
| No | `ExtractJSONStructure` | (*JSONValue, *Buffer, Structure [, Flags]) - Extract a JSON value into the specified structure. |
| No | `FreeJSON` | (#JSON) - Free JSON data. |
| No | `GetJSONBoolean` | (*JSONValue) - Return the JSON value as a boolean. |
| No | `GetJSONDouble` | (*JSONValue) - Return the JSON value as a double. |
| No | `GetJSONElement` | (*JSONValue, Index) - Return an element in a JSON array. |
| No | `GetJSONFloat` | (*JSONValue) - Return the JSON value as a float. |
| No | `GetJSONInteger` | (*JSONValue) - Return the JSON value as an integer. |
| No | `GetJSONMember` | (*JSONValue, Key$) - Return a member of a JSON object. |
| No | `GetJSONQuad` | (*JSONValue) - Return the JSON value as a quad. |
| No | `GetJSONString` | (*JSONValue) - Return the JSON value as a string. |
| No | `InsertJSONArray` | (*JSONValue, Array()) - Insert the specified Array() into the JSON value. |
| No | `InsertJSONList` | (*JSONValue, List()) - Insert the specified List() into the JSON value. |
| No | `InsertJSONMap` | (*JSONValue, Map()) - Insert the specified Map() into the JSON value. |
| No | `InsertJSONStructure` | (*JSONValue, *Buffer, Structure) - Insert the specified structure data into the JSON value. |
| No | `IsJSON` | (#JSON) - Check if #JSON represents valid JSON data. |
| No | `JSONArraySize` | (*JSONValue) - Return the number of elements in a JSON array. |
| No | `JSONErrorLine` | () - Get the input line of the most recent JSON parsing error. |
| No | `JSONErrorMessage` | () - Get a string description of the most recent JSON parsing error. |
| No | `JSONErrorPosition` | () - Get the input position of the most recent JSON parsing error. |
| No | `JSONMemberKey` | (*JSONValue) - Get the key of the current examined member in the JSON object. |
| No | `JSONMemberValue` | (*JSONValue) - Get the value of the current examined member in the JSON object. |
| No | `JSONObjectSize` | (*JSONValue) - Return the number of members in a JSON object. |
| No | `JSONType` | (*JSONValue) - Return the type of a JSON value. |
| No | `JSONValue` | (#JSON) - Return the main value of JSON data. |
| No | `LoadJSON` | (#JSON, FileName$ [, Flags]) - Load JSON data from a file. |
| No | `NextJSONMember` | (*JSONValue) - Get the next member in the examined JSON object. |
| No | `ParseJSON` | (#JSON, Input$ [, Flags]) - Parse JSON data from a string. |
| No | `RemoveJSONElement` | (*JSONValue, Index) - Remove an element from a JSON array. |
| No | `RemoveJSONMember` | (*JSONValue, Key$) - Remove a member from a JSON object. |
| No | `ResizeJSONElements` | (*JSONValue, Size) - Resize a JSON array. |
| No | `SaveJSON` | (#JSON, FileName$ [, Flags]) - Save JSON data to a file. |
| No | `SetJSONArray` | (*JSONValue) - Set a JSON value to contain an (empty) array. |
| No | `SetJSONBoolean` | (*JSONValue, Value) - Set a JSON value to contain a boolean. |
| No | `SetJSONDouble` | (*JSONValue, Value.d) - Set a JSON value to contain a double. |
| No | `SetJSONFloat` | (*JSONValue, Value.f) - Set a JSON value to contain a float. |
| No | `SetJSONInteger` | (*JSONValue, Value) - Set a JSON value to contain an integer. |
| No | `SetJSONNull` | (*JSONValue) - Set a JSON value to contain a null. |
| No | `SetJSONObject` | (*JSONValue) - Set a JSON value to contain an (empty) object. |
| No | `SetJSONQuad` | (*JSONValue, Value.q) - Set a JSON value to contain a quad. |
| No | `SetJSONString` | (*JSONValue, String$) - Set a JSON value to contain a string. |

## Keyboard

0/6 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ExamineKeyboard` | () - Update the keyboard state. |
| No | `InitKeyboard` | () - Initialize the keyboard environment for later use. |
| No | `KeyboardInkey` | () - Return the last typed character. |
| No | `KeyboardMode` | (Mode) - Change the way the keyboard keys are interpreted. |
| No | `KeyboardPushed` | (KeyID) - Return 1 if the specified key is currently pushed. |
| No | `KeyboardReleased` | (KeyID) - Return 1 if the specified key has been pressed (pushed and released). |

## Language/Compiler

0/5 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `Bool` | (Expression) - Test the expression and return #True or #False. |
| No | `Defined` | (Name, Type) - Returns 1 if the object is defined, or 0 if not. |
| No | `OffsetOf` | (Structure\Field) - Returns the offset of the specified structure field. |
| No | `SizeOf` | (Structure) - Returns the size of the given structure. |
| No | `Subsystem` | (Name) - Check if a subsystem is in use. |

## Library

0/14 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CallCFunction` | (#Library, FunctionName$, [Arg1, [Arg2, ...]]) - Call a C function using its name. |
| No | `CallCFunctionFast` | (*Function, [Arg1, [Arg2, ...]]) - Call a C function quickly, using its pointer. |
| No | `CallFunction` | (#Library, FunctionName$, [Arg1, [Arg2, ...]]) - Call a function using its name. |
| No | `CallFunctionFast` | (*Function, [Arg1, [Arg2, ...]]) - Call a function quickly, using its pointer. |
| No | `CloseLibrary` | (#Library) - Close the specified library. |
| No | `CountLibraryFunctions` | (#Library) - Count the total number of functions available in the #Library. |
| No | `ExamineLibraryFunctions` | (#Library) - Start to examine the functions contained in the #Library. |
| No | `GetFunction` | (#Library, FunctionName$) - Check if a function exists in the #Library. |
| No | `IsLibrary` | (#Library) - Test if the given '#Library' is an initialized library object. |
| No | `LibraryFunctionAddress` | () - Get the function address which is being examined. |
| No | `LibraryFunctionName` | () - Get the function name which is being examined. |
| No | `LibraryID` | (#Library) - Return the unique ID of the specified library. |
| No | `NextLibraryFunction` | () - Go to the next function in the library being examined. |
| No | `OpenLibrary` | (#Library, Filename$) - Open a new library (DLL). |

## Light

0/24 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CopyLight` | (#Light, #NewLight) - Copy the specified #Light to the #NewLight. |
| No | `CreateLight` | (#Light, Color [, x, y, z [, Flags]]) - Create a new #Light. |
| No | `DisableLightShadows` | (#Light, Disable) |
| No | `FreeLight` | (#Light) - Free the specified #Light. |
| No | `GetLightColor` | (#Light, Type) - Get the #Light color value. |
| No | `HideLight` | (#Light, State) - Hide or show the specified #Light. |
| No | `IsLight` | (#Light) - Test if the given '#Light' is an initialized light object. |
| No | `LightAttenuation` | (#Light, Range, Attenuation) - Change the #Light attenuation. |
| No | `LightDirection` | (#Light, x, y, z) - Change the #Light direction. |
| No | `LightDirectionX` | (#Light [, Mode]) - Return the x direction of the #Light. |
| No | `LightDirectionY` | (#Light [, Mode]) - Return the y direction of the #Light. |
| No | `LightDirectionZ` | (#Light [, Mode]) - Return the z direction of the #Light. |
| No | `LightID` | (#Light) - Return the unique #Light ID. |
| No | `LightLookAt` | (#Light, x, y, z) |
| No | `LightPitch` | (#Light [, Mode]) - Return the pitch of the #Light. |
| No | `LightRoll` | (#Light [, Mode]) - Return the roll of the #Light. |
| No | `LightX` | (#Light [, Mode]) - Return the x position of the #Light. |
| No | `LightY` | (#Light [, Mode]) - Return the y position of the #Light. |
| No | `LightYaw` | (#Light [, Mode]) - Return the yaw of the #Light. |
| No | `LightZ` | (#Light [, Mode]) - Return the z position of the #Light. |
| No | `MoveLight` | (#Light, x, y, z [, Mode]) - Move the #Light. |
| No | `RotateLight` | (#Light, x, y, z [, Mode]) - Rotate the #Light. |
| No | `SetLightColor` | (#Light, Type, Color) - Change the #Light color value. |
| No | `SpotLightRange` | (#Light, InnerAngle, OutterAngle [, FallOff]) |

## LinkedList

0/24 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddElement` | (List()) - Add a new empty element after the current element. |
| No | `ChangeCurrentElement` | (List(), *NewElement) - Change the current element of the specified list to the given new element. |
| No | `ClearList` | (List()) - Clear all the elements in this list and release their memory. |
| No | `CompareList` | (List1(), List2() [, Flags]) - Compare both lists and return 1 if they are the same or 0 if an element differs. |
| No | `CopyList` | (List(), DestinationList()) - Copy all the elements from the list to the destination list to have two identical lists. |
| No | `DeleteElement` | (List() [, Flags]) - Remove the current element from the list. |
| No | `ExtractList` | (List(), Text$ [, Separator$ [, Limit]]) - Split 'Text$' into 'List()' using 'Separator$' as the field separator (default ",") and return the number of elements found. If 'Limit' is greater than zero, at most 'Limit' elements are extracted, the last one containing the remainder of the string. 'List()' is cleared first, and has to be of String, Integer or Double type. |
| No | `FirstElement` | (List()) - Change the current list element to the first list element. |
| No | `FreeList` | (List()) - Free the list. 'NewList' has to be called to use it again. |
| No | `InsertElement` | (List()) - Insert a new empty element before the current element. |
| No | `JoinList` | (List() [, Separator$ [, Limit]]) - Join all the elements of 'List()' into a single string, using 'Separator$' between each element (default ","). If 'Limit' is greater than zero, only the first 'Limit' elements are joined. 'List()' has to be of String, Integer or Double type. |
| No | `LastElement` | (List()) - Change the current list element to the last list element. |
| No | `ListIndex` | (List()) - Find out the position of the current element in the list. First Element is 0. |
| No | `ListSize` | (List()) - Count how many elements there are in the list. |
| No | `MergeLists` | (List(), DestinationList() [, Location]) - Move all elements from List() to DestinationList(). |
| No | `MoveElement` | (List(), Location [, *RelativeElement]) - Move the current element to a different position in the list. |
| No | `NextElement` | (List()) - Move from the current element to the next element in the list. |
| No | `PopListPosition` | (List()) - Restore the current list position. |
| No | `PreviousElement` | (List()) - Move from the current element to the previous element in the list. |
| No | `PushListPosition` | (List()) - Save the current list position for later use. |
| No | `ResetList` | (List()) - Reset the current list element to be before the first element. |
| No | `SelectElement` | (List(), Position) - Change the current list element to the element at the specified position. |
| No | `SplitList` | (List(), DestinationList() [, KeepCurrent]) - Split List() into List() and DestinationList(). |
| No | `SwapElements` | (List(), *FirstElement, *SecondElement) - Swap the position of the specified elements. |

## Mail

0/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddMailAttachment` | (#Mail, Description$, Filename$ [, MimeType$]) - Add a file attachment. |
| No | `AddMailAttachmentData` | (#Mail, Description$, *Buffer, Size [, MimeType$]) - Add a file attachment from a memory area. |
| No | `AddMailRecipient` | (#Mail, Address$, Flags) - Add a recipient to the specified #Mail. |
| No | `CreateMail` | (#Mail, From$, Subject$ [, Encoding]) - Create a new mail. |
| No | `FreeMail` | (#Mail) - Free the previously created mail. |
| No | `GetMailAttribute` | (#Mail, Attribute) - Return the specified #Mail attribute. |
| No | `GetMailBody` | (#Mail) - Return the mail body. |
| No | `IsMail` | (#Mail) - Test if the given '#Mail' is an initialized mail object. |
| No | `MailProgress` | (#Mail) - Return the current progress of a mail being sent. |
| No | `RemoveMailRecipient` | (#Mail [, Address$ [, Flags]]) - Remove a recipient from the specified #Mail. |
| No | `SendMail` | (#Mail, Smtp$ [, Port [, Flags [, User$, Password$]]]) - Send the mail. |
| No | `SetMailAttribute` | (#Mail, Attribute, Value$) - Change the specified #Mail attribute. |
| No | `SetMailBody` | (#Mail, Body$) - Change the mail body. |

## Map

0/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddMapElement` | (Map(), Key$ [, Flags]) - Add a new empty element in the map. |
| No | `ClearMap` | (Map()) - Clear all the elements in this map and release their memory. |
| No | `CompareMap` | (Map1(), Map2() [, Flags]) - Compare both maps and return 1 if they are the same or 0 if an element or its value differs. |
| No | `CopyMap` | (Map(), DestinationMap()) - Copy all the elements from the map to the destination map to have two identical maps. |
| No | `DeleteMapElement` | (Map() [, Key$]) - Remove the current element from the map. |
| No | `FindMapElement` | (Map(), Key$) - Find the specified element in the map. |
| No | `FreeMap` | (Map()) - Free the map. 'NewMap' has to be called to use it again. |
| No | `MapKey` | (Map()) - Return the key of the current map element. |
| No | `MapSize` | (Map()) - Return the number of elements in the map. |
| No | `NextMapElement` | (Map()) - Move from the current element to the next element in the map. |
| No | `PopMapPosition` | (Map()) - Restore the current map position. |
| No | `PushMapPosition` | (Map()) - Save the current map position for later use. |
| No | `ResetMap` | (Map()) - Reset the current map element before the first element. |

## Material

0/33 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddMaterialLayer` | (#Material, TextureID [, Mode [, TextureCoordinateIndex]]) - Add a new texture layer to the #Material. |
| No | `CopyMaterial` | (#Material, #NewMaterial) - Copy the specified #Material on the #NewMaterial. |
| No | `CountMaterialLayers` | (#Material) - Return the number of texture layers the #Material contains. |
| No | `CreateAnimatedMaterial` | (#Material, TextureArray(), Duration.f) - Create a new animated #Material based on the specified texture array. |
| No | `CreateMaterial` | (#Material, TextureID [, Color]) - Create a new #Material based on the specified texture and color. |
| No | `CreateShader` | (ShaderID, VertexProgram$, FragmentProgram$) - Create a new shader. |
| No | `CreateShaderMaterial` | (#Material, ShaderID) - Create a new #Material using the specified shader. |
| No | `DisableMaterialLighting` | (#Material, Disable) - Disable the dynamic lighting on the #Material. |
| No | `FreeMaterial` | (#Material) - Free the specified #Material. |
| No | `GetMaterialAttribute` | (#Material, Attribute [, Layer]) - Get the specified #Material attribute. |
| No | `GetMaterialColor` | (#Material, Type) - Get the specified #Material color. |
| No | `GetScriptMaterial` | (#Material, Name$) - Get a material defined in script files. |
| No | `IsMaterial` | (#Material) - Test if the given '#Material' is an initialized material object. |
| No | `MaterialAnimation` | (#Material, TextureName$, NbFrames, Time) - Set an animated texture for the material. |
| No | `MaterialBlendingMode` | (#Material, Mode) - Change the #Material blending mode. |
| No | `MaterialCullingMode` | (#Material, Mode) - Change the #Material culling mode. |
| No | `MaterialFilteringMode` | (#Material, Mode [, MaxAnisotropy]) - Change the texture filtering mode for the #Material. |
| No | `MaterialFog` | (#Material, Color, Intensity, StartDistance, EndDistance) - Display a fog effect on the material. |
| No | `MaterialID` | (#Material) - Return the unique #Material ID. |
| No | `MaterialShaderAutoParameter` | (#Material, ProgramType, ParameterName$, ParameterType, ExtraValue.f) - Set the specified shader parameter value. |
| No | `MaterialShaderParameter` | (#Material, ProgramType, ParameterName$, ParameterType, v1.f, v2.f, v3.f, v4.f) - Set the specified shader parameter value. |
| No | `MaterialShaderTexture` | (#Material, TextureID1, TextureID2, TextureID3, TextureID4) - Set the textures to use for the shader. |
| No | `MaterialShadingMode` | (#Material, Mode) - Change the shading mode for the #Material. |
| No | `MaterialShininess` | (#Material, Shininess [, SpecularColor]) - Change the #Material shininess and specular color. |
| No | `MaterialTextureAliases` | (#Material, TextureID1, TextureID2, TextureID3, TextureID4) - Bind texture for use in scripts. |
| No | `ReloadMaterial` | (MaterialName$, Filename$, ParseScript) - Reload the specified material. |
| No | `RemoveMaterialLayer` | (#Material) - Remove the top most texture layer. |
| No | `ResetMaterial` | (ObjectType) - Reload the materials for all the specified objects. |
| No | `RotateMaterial` | (#Material, Angle, Mode [, Layer]) - Rotate the #Material texture. |
| No | `ScaleMaterial` | (#Material, x, y [, Layer]) - Scale the #Material texture. |
| No | `ScrollMaterial` | (#Material, x, y, Mode [, Layer]) - Scroll the #Material texture. |
| No | `SetMaterialAttribute` | (#Material, Attribute, Value [, Layer]) - Set the specified #Material attribute. |
| No | `SetMaterialColor` | (#Material, Type, Color) - Change the specified #Material color. |

## Math

17/33 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `Abs` | (Value) - Return the absolute value (no sign) of the given float value. |
| Yes | `ACos` | (Number) - Return the arc cosine of the specified number, in radian. |
| No | `ACosH` | (Number) - Return the area hyperbolic cosine of the specified number. |
| Yes | `ASin` | (Number) - Return the arc sine of the specified number, in radian. |
| No | `ASinH` | (Number) - Return the area hyperbolic sine of the specified number. |
| Yes | `ATan` | (Number) - Return the arc tangent of the specified number, in radian. |
| Yes | `ATan2` | (x, y) - Return the Arc-Tangent of x/y, in radian. |
| No | `ATanH` | (Number) - Return the area hyperbolic tangent of the specified number. |
| Yes | `Cos` | (Angle) - Return the cosine of the specified angle. |
| No | `CosH` | (Angle) - Return the hyperbolic cosine of the specified angle. |
| No | `Degree` | (Angle) - Convert the given angle from radian to degree. |
| Yes | `Exp` | (Exponent) - Return the e^Exponent. |
| No | `Infinity` | () - Return the float value for positive infinity. |
| Yes | `Int` | (Float) - Return the integer part of the specified value. |
| No | `IntQ` | (Float) - Return the integer part of the specified value as a quad. |
| No | `IsInfinity` | (Number) - Return nonzero if the given Number represents positive or negative infinity. |
| No | `IsNaN` | (Number) - Return nonzero if the given Number is the special "Not a Number" value. |
| Yes | `Log` | (Value) - Return the Log of the value. |
| Yes | `Log10` | (x) - Return the Log in base 10 of the value. |
| No | `Mod` | (Number, Divisor) - Return the remainder of Number/Divisor. |
| No | `NaN` | () - Return the special "Not a Number" value. |
| Yes | `Pow` | (Number, Exponent) - Return the Number^Power. |
| No | `Radian` | (Angle) - Convert the given angle from degree to radian. |
| Yes | `Random` | (Maximum [, Minimum]) - Return a random number between 0 (unless 'Minimum' is specified) and the 'Maximum' value. |
| No | `RandomData` | (*Buffer, Length) - Fill the buffer with bytes from the random number generator. |
| Yes | `RandomSeed` | (Seed) - Reinitialize the random seed, with the specified value. |
| Yes | `Round` | (Number, Mode) - Round the specified float number according to the given mode. |
| No | `Sign` | (Number) - Return 1, 0 or -1 depending on the sign of the given number. |
| Yes | `Sin` | (Angle) - Return the sine of the specified angle. |
| No | `SinH` | (Angle) - Return the hyperbolic sine of the specified angle. |
| Yes | `Sqr` | (Value) - Return the square root of the specified number. |
| Yes | `Tan` | (Angle) - Return the tangent of the specified angle. |
| No | `TanH` | (Angle) - Return the hyperbolic tangent of the specified angle. |

## Math3D

0/12 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ConvertLocalToWorldPosition` | (ObjectID, x, y, z) - Convert local position of the object into world coordinate. |
| No | `ConvertWorldToLocalPosition` | (ObjectID, x, y, z) - Convert world position of the object into local coordinate. |
| No | `FetchOrientation` | (ObjectID [, Mode]) - Fetch the orientation for the specified object (use GetX(), GetY(), GetZ() and GetW() to get the values). |
| No | `GetW` | () - Get the 'w' value of the last call. |
| No | `GetX` | () - Get the 'x' value of the last call. |
| No | `GetY` | () - Get the 'y' value of the last call. |
| No | `GetZ` | () - Get the 'z' value of the last call. |
| No | `Pitch` | (ObjectID, Value.f, Mode) - Pitch the specified object. |
| No | `Roll` | (ObjectID, Value.f, Mode) - Roll the specified object. |
| No | `SetOrientation` | (ObjectID, x, y, z, w) - Set the orientation for the specified object. |
| No | `SetRenderQueue` | (ObjectID, Queue [, Priority]) - Change the render order of the specified object. |
| No | `Yaw` | (ObjectID, Value.f, Mode) - Yaw the specified object. |

## Memory

20/40 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AllocateMemory` | (Size [, Flags]) - Allocate a contiguous memory area according to the specified size and return the buffer start. |
| No | `AllocateStructure` | (StructureName) |
| No | `ClearStructure` | (*Structure, Structure) - Clear the specified *Structure. |
| No | `CompareMemory` | (*Buffer1, *Buffer2, Size) - Compare two memory areas and return 1 if its equal or 0 if it doesn't match. |
| No | `CompareMemoryString` | (*String1, *String2 [, Mode [, Length [, Flags]]]) - Return 0 if *String1 equals *String2, -1 or 1 if *String1 is below or above *String2. |
| No | `CompareStructure` | (*Buffer1, *Buffer2, Structure [, Flags]) - Compare two structures and return 1 if they are equal or 0 if not. |
| No | `CopyMemory` | (SourceMemory, DestinationMemory, Size) - Copy a memory area from 'SourceMemory' to 'DestinationMemory'. |
| No | `CopyMemoryString` | (*String [, DestinationMemory]) - Copy the *String to destination memory address or after the last one. |
| No | `CopyStructure` | (*Source, *Destination, Structure) - Copy the *Source structure to the *Destination structure. |
| No | `FillMemory` | (Memory, Size [, Value [, Type]]) - Fill a memory area with the specified value. |
| No | `FreeMemory` | (Memory) - Free the specified memory buffer. |
| No | `FreeStructure` | (*Structure) |
| No | `InitializeStructure` | (*Structure, Structure) - Initialize the specified *Structure. |
| No | `MemorySize` | (Memory) - Return the size of the specified memory block. |
| No | `MemoryStringLength` | (*String [, Flags]) - Calculate the length of a memory string. |
| No | `MoveMemory` | (SourceMemory, DestinationMemory, Size) - Move a memory area from 'SourceMemory' to 'DestinationMemory'. |
| Yes | `PeekA` | (Memory) - Read an ascii character (1 byte) from the specified memory address. |
| Yes | `PeekB` | (*MemoryBuffer) - Read a byte number from the address. |
| Yes | `PeekC` | (Memory) - Read a character (1 byte in ascii, 2 bytes in unicode) from the specified memory address. |
| Yes | `PeekD` | (Memory) - Read a double (8 bytes) from the specified memory address. |
| Yes | `PeekF` | (*MemoryBuffer) - Read a float (4 bytes) from the specified memory address. |
| No | `PeekI` | (Memory) - Read an integer (8 bytes) from the specified memory address. |
| Yes | `PeekL` | (*MemoryBuffer) - Read a long (4 bytes) number from the specified memory address. |
| Yes | `PeekQ` | (Memory) - Read a quad (8 bytes) from the specified memory address. |
| Yes | `PeekS` | (Memory [, Length [, Format]]) - Read a null terminated string at the specified memory address. |
| Yes | `PeekU` | (Memory) - Read an unicode character (2 bytes) from the specified memory address. |
| Yes | `PeekW` | (*MemoryBuffer) - Read a word (2 bytes) number from the specified memory address. |
| Yes | `PokeA` | (*MemoryBuffer, Ascii.a) - Write an ascii character (1 byte) to the specified memory address. |
| Yes | `PokeB` | (*MemoryBuffer, Number) - Write a byte number to the specified memory address. |
| Yes | `PokeC` | (*MemoryBuffer, Character.c) - Write a character (1 byte in ascii, 2 bytes in unicode) to the specified memory address. |
| Yes | `PokeD` | (*MemoryBuffer, Double.d) - Write a double (8 bytes) to the specified memory address. |
| Yes | `PokeF` | (*MemoryBuffer, Number.f) - Write a float (4 bytes) to the specified memory address. |
| No | `PokeI` | (*MemoryBuffer, Number) - Write an integer (8 bytes) to the specified memory address. |
| Yes | `PokeL` | (*MemoryBuffer, Number) - Write a long (4 bytes) number to the specified memory address. |
| Yes | `PokeQ` | (*MemoryBuffer, Quad.q) - Write a quad (8 bytes) to the specified memory address. |
| Yes | `PokeS` | (*MemoryBuffer, Text$ [, Length [, Flags]]) - Write a string (including the ending '0') to the specified memory address. |
| Yes | `PokeU` | (*MemoryBuffer, Unicode.U) - Write an unicode character (2 bytes) to the specified memory address. |
| Yes | `PokeW` | (*MemoryBuffer, Number) - Write a word (2 bytes) number to the specified memory address. |
| No | `ReAllocateMemory` | (Memory, Size [, Flags]) - ReAllocate a contiguous memory area according to the specified size. |
| No | `ResetStructure` | (*Structure, Structure) - Reset the specified *Structure. |

## Menu

21/24 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `BindMenuEvent` | (#Menu, MenuItem, @Callback()) - Bind a menu item event to a callback. |
| Yes | `CloseSubMenu` | () - Close the actual sub menu and come back to the previous one. |
| Yes | `CreateImageMenu` | (#Menu, WindowID [, Flags]) - Create a new empty menu with images support. |
| Yes | `CreateMenu` | (#Menu, WindowID) - Create a new empty menu. |
| Yes | `CreatePopupImageMenu` | (#Menu [, Flags]) - Create a new empty popup menu with images support. |
| Yes | `CreatePopupMenu` | (#Menu) - Create a new empty popup menu. |
| Yes | `DisableMenuItem` | (#Menu, MenuItem, State) - Disable or enable a menu item. |
| No | `DisplayPopupMenu` | (#Menu, WindowID [, x, y]) - Display a previously created #PopupMenu. |
| Yes | `FreeMenu` | (#Menu) - Remove the specified menu from memory. |
| Yes | `GetMenuItemState` | (#Menu, MenuItem) - Retrieve the current menu item state. |
| Yes | `GetMenuItemText` | (#Menu, MenuItem) - Retrieve the current menu item text. |
| Yes | `GetMenuTitleText` | (#Menu, Index) - Retrieve the current menu title text. |
| Yes | `HideMenu` | (#Menu, State) - Hide or display the specified menu. |
| Yes | `IsMenu` | (#Menu) - Test if the given '#Menu' is an initialized menu object. |
| Yes | `MenuBar` | () - Create a separator bar in the current menu. |
| Yes | `MenuHeight` | () - Return the menu bar height, in pixels. |
| Yes | `MenuID` | (#Menu) - Return the unique ID which identifies the menu in the operating system. |
| Yes | `MenuItem` | (MenuItemID, Text$ [, ImageID]) - Create a new item entry for the menu. |
| Yes | `MenuTitle` | (Title$) - Create a new title entry for the menu. |
| Yes | `OpenSubMenu` | (Text$ [, ImageID]) - Create an empty sub-menu entry in the current menu. |
| Yes | `SetMenuItemState` | (#Menu, MenuItem, State) - Change the specified #Menu item state. |
| Yes | `SetMenuItemText` | (#Menu, MenuItem, Text$) - Change the specified #Menu item text. |
| Yes | `SetMenuTitleText` | (#Menu, Index, Text$) - Change the specified #Menu title text. |
| No | `UnbindMenuEvent` | (#Menu, MenuItem, @Callback()) - Unbind a menu item event callback. |

## Mesh

0/46 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddMeshManualLOD` | (#Mesh, #MeshLOD, Distance) - Manually build several levels of detail for the #Mesh. |
| No | `AddSubMesh` | ([Type]) |
| No | `BuildMeshLOD` | (#Mesh [, Number, Distance.f, Reduction.f]) - Automatically build several levels of detail for the #Mesh. |
| No | `BuildMeshShadowVolume` | (#Mesh) |
| No | `BuildMeshTangents` | (#Mesh) |
| No | `CopyMesh` | (#Mesh, #NewMesh) - Copy the specified #Mesh on the #NewMesh. |
| No | `CreateCapsule` | (#Mesh, Radius.f, Height.f [, NbRings, NbSegments, NbHeightSegments]) - Create a new capsule mesh. |
| No | `CreateCone` | (#Mesh, Radius.f, Height.f [, NbBaseSegments, NbHeightSegments]) - Create a new cone mesh. |
| No | `CreateCube` | (#Mesh, Size) - Create a new cube mesh. |
| No | `CreateCylinder` | (#Mesh, Radius.f, Height.f [, NbBaseSegments, NbHeightSegments, CloseTop]) - Create a new cylinder mesh. |
| No | `CreateDataMesh` | (#Mesh, Data() [, Mode]) - Create a new #Mesh based on an array of PB_MeshVertex. |
| No | `CreateIcoSphere` | (#Mesh, Radius.f [, Iterations]) - Create a new icosphere mesh. |
| No | `CreateLine3D` | (#Mesh, x, y, z, Color, x2, y2, z2, Color2) |
| No | `CreateMesh` | (#Mesh [, Type [, Mode]]) - Create a new empty #Mesh. |
| No | `CreatePlane` | (#Mesh, TileSizeX.f, TileSizeZ.f, TileCountX, TileCountZ, TextureRepeatCountX.f, TextureRepeatCountZ.f) - Create a new plane mesh. |
| No | `CreateSphere` | (#Mesh, Radius.f [, NbSegments, Rings]) - Create a new sphere mesh. |
| No | `CreateTorus` | (#Mesh, Radius.f, SectionRadius.f [, NbSectionSegments, NbCircleSegments]) - Create a new torus mesh. |
| No | `CreateTube` | (#Mesh, OuterRadius.f, InnerRadius.f, Height.f [, NbBaseSegments, NbHeightSegments]) - Create a new tube mesh. |
| No | `FinishMesh` | (StaticMesh) |
| No | `FreeMesh` | (#Mesh) - Free the specified #Mesh. |
| No | `GetMeshData` | (#Mesh, SubMesh, DataArray(), Flags, FirstIndex, LastIndex) |
| No | `IsMesh` | (#Mesh) - Test if the given '#Mesh' is an initialized mesh object. |
| No | `LoadMesh` | (#Mesh, Filename$) - Load a new #Mesh. |
| No | `MeshAnimationList` | (#Mesh) |
| No | `MeshBoneList` | (#Mesh) |
| No | `MeshDirectAdd` | (#Mesh, VertexArray(), FaceArray(), Type, MaterialID) |
| No | `MeshFace` | (Vertex1, Vertex2, Vertex3 [, Vertex4]) |
| No | `MeshID` | (#Mesh) - Return the unique #Mesh ID. |
| No | `MeshIndex` | (Index) |
| No | `MeshIndexCount` | (#Mesh [, SubMesh]) |
| No | `MeshRadius` | (#Mesh) |
| No | `MeshVertex` | (x, y, z, u, v, Color [, NormalX, NormalY, NormalZ]) |
| No | `MeshVertexColor` | (Color) |
| No | `MeshVertexCount` | (#Mesh [, SubMesh]) |
| No | `MeshVertexNormal` | (x, y, z) |
| No | `MeshVertexPosition` | (x, y, z) |
| No | `MeshVertexTangent` | (x, y, z) |
| No | `MeshVertexTextureCoordinate` | (u [, v [, w]]) |
| No | `NormalizeMesh` | (#Mesh [, SubMesh]) |
| No | `SaveMesh` | (#Mesh, Filename$) |
| No | `SetMeshData` | (#Mesh, SubMesh, DataArray(), Flags, FirstIndex, LastIndex) |
| No | `SetMeshMaterial` | (#Mesh, MaterialID [, SubMesh]) |
| No | `SubMeshCount` | (#Mesh) |
| No | `TransformMesh` | (#Mesh, x, y, z, ScaleX, ScaleY, ScaleZ, RotateX, RotateY, RotateZ [, SubMesh]) |
| No | `UpdateMesh` | (#Mesh, Index) |
| No | `UpdateMeshBoundingBox` | (#Mesh [, MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ]) |

## Mouse

0/10 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ExamineMouse` | () - Update the mouse state. |
| No | `InitMouse` | () - Initialize the Mouse environment for later use. |
| No | `MouseButton` | (ButtonNumber) - Return 0 if the specified button is not pressed, else the button is pressed. |
| No | `MouseDeltaX` | () - Return the mouse X movement (in pixel) since the last call of this function. |
| No | `MouseDeltaY` | () - Return the mouse Y movement (in pixel) since the last call of this function. |
| No | `MouseLocate` | (x, y) - Change the mouse X,Y position to the new specified one. |
| No | `MouseWheel` | () - Return the number of positions since the last call. |
| No | `MouseX` | () - Return the actual mouse X position (in pixel) on the current screen. |
| No | `MouseY` | () - Return the actual mouse Y position (in pixel) on the current screen. |
| No | `ReleaseMouse` | (State) - Release or acquire the exclusive mouse access. |

## Movie

0/16 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `FreeMovie` | (#Movie) - Free the specified #Movie and remove it from memory. |
| No | `InitMovie` | () - Initialize the Movie environment for later use. |
| No | `IsMovie` | (#Movie) - Test if the given '#Movie' is an initialized movie object. |
| No | `LoadMovie` | (#Movie, Filename$) - Try to open and prepare a movie for later playback. |
| No | `MovieAudio` | (#Movie, Volume, Balance) - Control the audio stream of the movie. |
| No | `MovieHeight` | (#Movie) - Return the height (in pixel) of the movie. |
| No | `MovieInfo` | (#Movie, Flags) - Return additional information about the movie. |
| No | `MovieLength` | (#Movie) - Return the length (in frames) of the movie. |
| No | `MovieSeek` | (#Movie, Frame) - Change the movie position to the given frame. |
| No | `MovieStatus` | (#Movie) - Get the status of the movie (-1: paused, 0: stopped, >0: playing). |
| No | `MovieWidth` | (#Movie) - Return the width (in pixel) of the movie. |
| No | `PauseMovie` | (#Movie) - Pause the movie playback. |
| No | `PlayMovie` | (#Movie, WindowID) - Start to play a movie previously loaded with LoadMovie() on the specified window. |
| No | `ResizeMovie` | (#Movie, x, y, Width, Height) - Resize and move the movie display area on the movie window. |
| No | `ResumeMovie` | (#Movie) - Start playing the movie again, after a PauseMovie() command. |
| No | `StopMovie` | (#Movie) - Stop playing the movie. |

## Music

0/10 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CatchMusic` | (#Music, *Buffer, Length) - Create a new #Music found at the specified memory address. |
| No | `FreeMusic` | (#Music) - Stop and remove the specified module previously loaded with the LoadMusic() command from memory. |
| No | `GetMusicPosition` | (#Music) - Return the current pattern position of the playing module. |
| No | `GetMusicRow` | (#Music) - Return the row position in the pattern currently played. |
| No | `IsMusic` | (#Music) - Test if the given '#Music' is an initialized module object. |
| No | `LoadMusic` | (#Music, Filename$) - Load the specified module into memory. |
| No | `MusicVolume` | (#Music, Volume.f) - Change the current module volume in realtime. |
| No | `PlayMusic` | (#Music) - Start to play the specified module previously loaded with the LoadMusic() command. |
| No | `SetMusicPosition` | (#Music, Position) - Change the current pattern position of the playing module to the new one. |
| No | `StopMusic` | (#Music) - Stop the current module (if it was playing). |

## Network

0/23 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CloseNetworkConnection` | (Connection) - Close the specified client connection and send a notification to the server. |
| No | `CloseNetworkServer` | (#Server) - Shut down the specified server. |
| No | `ConnectionID` | (Connection) - Return the unique system identifier of the connection. |
| No | `CreateNetworkServer` | (ServerID, Port [, Flags [, BoundIP$]]) - Create a new network server on the local computer at the specified port. |
| No | `EventClient` | () - Server side: lets you know which client has sent the data. |
| No | `EventServer` | () - Server side: lets you know on which server the data has been received. |
| No | `ExamineIPAddresses` | ([Format]) - Start to examine the host's current IPs. |
| No | `FreeIP` | (IP) - Free the IPv6. |
| No | `GetClientIP` | (Client) - Return the IP of the specified client. |
| No | `GetClientPort` | (Client) - Return the Port of the specified client. |
| No | `Hostname` | () - Return the computer hostname. |
| No | `IPAddressField` | (IPAddress, Field [, Format]) - Return the IP Address field value. |
| No | `IPString` | (IPAddress [, Format]) - Return the string representation of the given IP address. |
| No | `MakeIPAddress` | (Field0, Field1, Field2, Field3 [, Field4, Field5, Field6, Field7]) - Return the value for the specified IP. |
| No | `NetworkClientEvent` | (Connection) - Check if an event occurred on a client. |
| No | `NetworkServerEvent` | ([ServerID]) - Check if an event occurred on a server. |
| No | `NextIPAddress` | () - Retrieve the next host IP address. |
| No | `OpenNetworkConnection` | (ServerName$, Port [, Flags [, TimeOut [, LocalIP$ [, LocalPort]]]]) - Try to open a connection on the specified server. |
| No | `ReceiveNetworkData` | (Client, *DataBuffer, DataBufferLength) - Receive raw data from the specified client. |
| No | `SendNetworkData` | (Client, *MemoryBuffer, Length) - Send raw data to the specified client. |
| No | `SendNetworkString` | (Client, String$ [, Format]) - Send a string to the specified client. |
| No | `ServerID` | (#Server) - Return the server system identifier. |
| No | `UseNetworkTLS` | ([PrivateKey$, Certificate$ [, CaCertificate$]]) - Enable and configure TLS support for the network library. |

## Node

0/17 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AttachNodeObject` | (#Node, ObjectID) - Attach an object to the #Node. |
| No | `CreateNode` | (#Node [, x, y, z]) - Create a new #Node. |
| No | `DetachNodeObject` | (#Node, ObjectID) - Detach the object of the #Node. |
| No | `FreeNode` | (#Node) - Free the specified #Node. |
| No | `IsNode` | (#Node) - Test if the given '#Node' is an initialized node object. |
| No | `MoveNode` | (#Node, x, y, z [, Mode]) - Move the #Node. |
| No | `NodeFixedYawAxis` | (#Node, Mode [, VectorX, VectorY, VectorZ]) - Change the way the yaw axis is managed. |
| No | `NodeID` | (#Node) - Return the unique #Node system identifier. |
| No | `NodeLookAt` | (#Node, x, y, z [, DirectionX, DirectionY, DirectionZ]) - Change the #Node orientation to look at the specified point. |
| No | `NodePitch` | (#Node [, Mode]) - Return the pitch of the #Node. |
| No | `NodeRoll` | (#Node [, Mode]) - Return the roll of the #Node. |
| No | `NodeX` | (#Node [, Mode]) - Return the x position of the #Node. |
| No | `NodeY` | (#Node [, Mode]) - Return the y position of the #Node. |
| No | `NodeYaw` | (#Node [, Mode]) - Return the yaw of the #Node. |
| No | `NodeZ` | (#Node [, Mode]) - Return the z position of the #Node. |
| No | `RotateNode` | (#Node, x, y, z [, Mode]) - Rotate the #Node. |
| No | `ScaleNode` | (#Node, x, y, z [, Mode]) - Scale the #Node. |

## NodeAnimation

0/23 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddNodeAnimationTime` | (#NodeAnimation, Time) - Add time to the specified #NodeAnimation. |
| No | `CreateNodeAnimation` | (#NodeAnimation, NodeID, Length, Interpolation, RotationInterpolation) - Create a new #NodeAnimation. |
| No | `CreateNodeAnimationKeyFrame` | (#NodeAnimation, Time, x, y, z) - Create a new keyframe for the #NodeAnimation. |
| No | `FreeNodeAnimation` | (#NodeAnimation) - Free the specified #NodeAnimation. |
| No | `GetNodeAnimationKeyFramePitch` | (#NodeAnimation, KeyFrame) - Get the keyframe pitch for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameRoll` | (#NodeAnimation, KeyFrame) - Get the keyframe roll for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameTime` | (#NodeAnimation, KeyFrame) - Get the keyframe time for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameX` | (#NodeAnimation, KeyFrame) - Get the x keyframe position for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameY` | (#NodeAnimation, KeyFrame) - Get the y keyframe position for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameYaw` | (#NodeAnimation, KeyFrame) - Get the keyframe yaw for the #NodeAnimation. |
| No | `GetNodeAnimationKeyFrameZ` | (#NodeAnimation, KeyFrame) - Get the z keyframe position for the #NodeAnimation. |
| No | `GetNodeAnimationLength` | (#NodeAnimation) - Get the #NodeAnimation length. |
| No | `GetNodeAnimationTime` | (#NodeAnimation) - Get the current #NodeAnimation time. |
| No | `GetNodeAnimationWeight` | (#NodeAnimation) - Get the #NodeAnimation weight. |
| No | `NodeAnimationStatus` | (#NodeAnimation) - Return the specified #NodeAnimation status. |
| No | `SetNodeAnimationKeyFramePosition` | (#NodeAnimation, KeyFrame, x, y, z) - Set the keyframe position for the #NodeAnimation. |
| No | `SetNodeAnimationKeyFrameRotation` | (#NodeAnimation, KeyFrame, x, y, z [, w, Mode]) - Set the keyframe rotation for the #NodeAnimation. |
| No | `SetNodeAnimationKeyFrameScale` | (#NodeAnimation, KeyFrame, x, y, z) - Set the keyframe scale for the #NodeAnimation. |
| No | `SetNodeAnimationLength` | (#NodeAnimation, Length) - Set the #NodeAnimation length. |
| No | `SetNodeAnimationTime` | (#NodeAnimation, Time) - Set the current #NodeAnimation time. |
| No | `SetNodeAnimationWeight` | (#NodeAnimation, Weight) - Set the #NodeAnimation weight. |
| No | `StartNodeAnimation` | (#NodeAnimation [, Flags]) - Start the specified #NodeAnimation. |
| No | `StopNodeAnimation` | (#NodeAnimation) - Stop the specified #NodeAnimation. |

## OnError

0/16 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ErrorAddress` | () - Get the code address at which the error happened. |
| No | `ErrorCode` | () - Return the current error code. |
| No | `ErrorFile` | () - Return the source filename in which the current error occurred. |
| No | `ErrorLine` | () - Return the line number at which the current error occurred. |
| No | `ErrorMessage` | ([ErrorCode]) - Return a string message for the current error or the given error number. |
| No | `ErrorRegister` | (Register) - Get the content of a register at the time of the error. |
| No | `ErrorTargetAddress` | () - Get the memory address for a memory access error. |
| No | `ExamineAssembly` | (*Address [, *EndAddress]) - Start disassembling code at the given address. |
| No | `InstructionAddress` | () - Get the address of the current decoded instruction. |
| No | `InstructionString` | () - Get a string representation of the current decoded instruction. |
| No | `NextInstruction` | () - Disassemble the next instruction. |
| No | `OnErrorCall` | (@ErrorHandler()) - Call the given error handler if an error occurs. |
| No | `OnErrorDefault` | () - Uninstall any error handler so the default OS action is taken on further errors. |
| No | `OnErrorExit` | () - Exit the program if an error occurs. |
| No | `OnErrorGoto` | (?LabelAddress) - Continue execution at the given label if an error occurs. |
| No | `RaiseError` | (ErrorNumber) - Raise the given error and call the error handler or exit. |

## Packer

0/19 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddPackDirectory` | (#Pack, PackedDirectoryName$) - Add an empty directory to pack file. |
| No | `AddPackFile` | (#Pack, Filename$, PackedFilename$) - Add and compress the file to pack file. |
| No | `AddPackMemory` | (#Pack, *Buffer, Size, PackedFilename$) - Add and compress the memory area to pack file. |
| No | `CatchPack` | (#Pack, *Buffer, Size [, PluginID]) - Read a pack file from memory. |
| No | `ClosePack` | (#Pack) - Close the specified pack. |
| No | `CompressMemory` | (*Buffer, Size, *Output, OutputSize [, PluginID [, Level]]) - Compress the content of the *Buffer into *Output. |
| No | `CreatePack` | (#Pack, Filename$ [, PluginID [, Level]]) - Create a new empty pack file. |
| No | `ExaminePack` | (#Pack) - Examine the pack content. |
| No | `IsPack` | (#Pack) - Test if the given '#Pack' is an initialized pack object. |
| No | `NextPackEntry` | (#Pack) - Go to the next pack entry. |
| No | `OpenPack` | (#Pack, Filename$ [, PluginID]) - Open a pack file. |
| No | `PackEntryDate` | (#Pack [, Type]) - Return the current pack entry date. |
| No | `PackEntryName` | (#Pack) - Return the current pack entry name. |
| No | `PackEntrySize` | (#Pack [, Mode]) - Return the current pack entry size. |
| No | `PackEntryType` | (#Pack) - Return the current pack entry type. |
| No | `PackerCallback` | (@Callback()) - Set a callback to monitor file compression and decompression. |
| No | `UncompressMemory` | (*Buffer, Size, *Output, OutputSize [, PluginID]) - Uncompress the content of the *Buffer into *Output. |
| No | `UncompressPackFile` | (#Pack, Filename$ [, PackedFilename$]) - Uncompress the current or the specified filename in the archive to the specified file. |
| No | `UncompressPackMemory` | (#Pack, *Buffer, Size [, PackedFilename$]) - Uncompress the current or the specified filename in the archive to memory. |

## PackerBriefLZ

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseBriefLZPacker` | () - Add BriefLZ support to the packer commandset. |

## PackerJCALG1

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseJCALG1Packer` | () - Add JCALG1 decompression support to the packer commandset. |

## PackerLZMA

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseLZMAPacker` | () - Add LZMA support to the packer commandset. |

## PackerTAR

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseTARPacker` | () - Add TAR support to the packer commandset. |

## PackerZip

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseZipPacker` | () - Add ZIP support to the packer commandset. |

## Particle

0/25 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CreateParticleEmitter` | (#ParticleEmitter, Width, Height, Depth, Type [, x, y, z]) - Create a new #ParticleEmitter at the given position. |
| No | `DisableParticleEmitter` | (#ParticleEmitter, State) - Disable or enable a particle emitter. |
| No | `FreeParticleEmitter` | (#ParticleEmitter) - Free the specified #ParticleEmitter. |
| No | `GetScriptParticleEmitter` | (#ParticleEmitter, Name$) - Get a particle emitter defined in a script file. |
| No | `HideParticleEmitter` | (#ParticleEmitter, Hide) - Hide or show the specified #ParticleEmitter. |
| No | `IsParticleEmitter` | (#ParticleEmitter) - Test if the given '#ParticleEmitter' is an initialized ParticleEmitter object. |
| No | `MoveParticleEmitter` | (#ParticleEmitter, x, y, z [, Mode]) - Move the #ParticleEmitter relative to its previous position. |
| No | `ParticleAcceleration` | (#ParticleEmitter, x, y, z) - Change the particles acceleration. |
| No | `ParticleAngle` | (#ParticleEmitter, StartRange, EndRange [, StartRangeSpeed, EndRangeSpeed]) - Change the particles angle. |
| No | `ParticleColorFader` | (#ParticleEmitter, RedRate, GreenRate, BlueRate, AlphaRate) - Change the particles color fader rate. |
| No | `ParticleColorRange` | (#ParticleEmitter, StartColor, EndColor) - Change the particles color range. |
| No | `ParticleEmissionRate` | (#ParticleEmitter, Rate) - Change the #ParticleEmitter emission rate (particles per second). |
| No | `ParticleEmitterAngle` | (#ParticleEmitter, Angle) - Change the particles angle. |
| No | `ParticleEmitterDirection` | (#ParticleEmitter, x, y, z) - Change the #ParticleEmitter direction. |
| No | `ParticleEmitterID` | (#ParticleEmitter) - Return the unique ID which identifies the particle emitter in the operating system. |
| No | `ParticleEmitterX` | (#ParticleEmitter [, Mode]) - Return the x position of the #ParticleEmitter. |
| No | `ParticleEmitterY` | (#ParticleEmitter [, Mode]) - Return the y position of the #ParticleEmitter. |
| No | `ParticleEmitterZ` | (#ParticleEmitter [, Mode]) - Return the z position of the #ParticleEmitter. |
| No | `ParticleMaterial` | (#ParticleEmitter, MaterialID) - Change the #ParticleEmitter material with the new one. |
| No | `ParticleScaleRate` | (#ParticleEmitter, ScaleRate) - Change the particles scale rate. |
| No | `ParticleSize` | (#ParticleEmitter, Width, Height) - Change the particles dimensions. |
| No | `ParticleSpeedFactor` | (#ParticleEmitter, SpeedFactor) - Change the particles speed factor. |
| No | `ParticleTimeToLive` | (#ParticleEmitter, MinimumTime, MaximumTime) - Change the particles time to live. |
| No | `ParticleVelocity` | (#ParticleEmitter, Mode, Value) - Change the particles velocity. |
| No | `ResizeParticleEmitter` | (#ParticleEmitter, x, y, z) - Resize the #ParticleEmitter. |

## Preference

0/27 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ClosePreferences` | () - Close the currently opened preference file. |
| No | `CreatePreferences` | (Filename$ [, Flags]) - Create a new empty preference file. |
| No | `ExaminePreferenceGroups` | () - Start examining the groups in the preference file. |
| No | `ExaminePreferenceKeys` | () - Start examining the keys in the current group of the preference file. |
| No | `FlushPreferenceBuffers` | () - Write the current preferences to disk. |
| No | `NextPreferenceGroup` | () - Go to the next group after a call to ExaminePreferenceGroups(). |
| No | `NextPreferenceKey` | () - Go to the next key after a call to ExaminePreferenceKeys(). |
| No | `OpenPreferences` | (Filename$ [, Flags [, Encoding]]) - Open and load a preference file. |
| No | `PreferenceComment` | (Comment$) - Add a comment to the current preference file. |
| No | `PreferenceGroup` | (GroupName$) - Search or create the group in the preference file. |
| No | `PreferenceGroupName` | () - Get the name of the currently examined preference group. |
| No | `PreferenceKeyName` | () - Get the name of the currently examined preference key. |
| No | `PreferenceKeyValue` | () - Get the value of the currently examined preference key as a string. |
| No | `ReadPreferenceDouble` | (Key$, DefaultValue) - Read the double value associated to the keyword. |
| No | `ReadPreferenceFloat` | (Key$, DefaultValue) - Read the value associated to the keyword. |
| No | `ReadPreferenceInteger` | (Key$, DefaultValue) - Read the value associated to the keyword. |
| No | `ReadPreferenceLong` | (Key$, DefaultValue) - Read the value associated to the keyword. |
| No | `ReadPreferenceQuad` | (Key$, DefaultValue) - Read the quad value associated to the keyword. |
| No | `ReadPreferenceString` | (Key$, DefaultValue$) - Read the value associated to the keyword. |
| No | `RemovePreferenceGroup` | (Name$) - Remove the group and all keys from the file. |
| No | `RemovePreferenceKey` | (Name$) - Remove the specified key from the current group. |
| No | `WritePreferenceDouble` | (Key$, Value) - Write the keyword and its associated double value. |
| No | `WritePreferenceFloat` | (Key$, Value) - Write the keyword and its associated value. |
| No | `WritePreferenceInteger` | (Key$, Value) - Write the keyword and its associated value. |
| No | `WritePreferenceLong` | (Key$, Value) - Write the keyword and its associated value. |
| No | `WritePreferenceQuad` | (Key$, Value) - Write the keyword and its associated quad value. |
| No | `WritePreferenceString` | (Key$, Value$) - Write the keyword and its associated value. |

## Printer

0/9 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `DefaultPrinter` | () - Use the default printer to print. |
| No | `NewPrinterPage` | () - Start a new page. |
| No | `PrinterOutput` | () - Return the printer drawing area, to be used with the 2DDrawing commands. |
| No | `PrinterPageHeight` | ()  - Return the height of the drawing area, in pixels. |
| No | `PrinterPageWidth` | () - Return the width of the drawing area, in pixels. |
| No | `PrinterVectorOutput` | ([Unit]) - Return the printer drawing area for vector drawing output. |
| No | `PrintRequester` | () - Open a print requester, necessary to print. |
| No | `StartPrinting` | (JobName$) - Initialize the printer and start a new print job. |
| No | `StopPrinting` | () - Finish the print session and send all the data to the printer. |

## Process

0/25 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AvailableProgramOutput` | (Program) - Return the number of bytes available for reading. |
| No | `CloseProgram` | (Program) - Close the interaction with the specified program. |
| No | `CountProgramParameters` | () - Get the number of parameters passed to this program. |
| No | `EnvironmentVariableName` | () - Get the name of the currently examined environment variable. |
| No | `EnvironmentVariableValue` | () - Get the value of the currently examined environment variable. |
| No | `ExamineEnvironmentVariables` | () - Start examining the environment block. |
| No | `GetEnvironmentVariable` | (Name$) - Return the content of the environment variable. |
| No | `IsProgram` | (Program) - Check if the specified program was created by RunProgram. |
| No | `KillProgram` | (Program) - Terminate the specified program. |
| No | `NextEnvironmentVariable` | () - Examine the next variable in the environment block. |
| No | `ProgramExitCode` | (Program) - Get the exitcode of the specified program. |
| No | `ProgramFilename` | () - Get the full path and filename of the executable of this program. |
| No | `ProgramID` | (Program) - Return the global Process ID for the running program. |
| No | `ProgramParameter` | ([Index]) - Get the next passed program parameter, or the parameter at Index. |
| No | `ProgramRunning` | (Program) - Check if the specified program is still running. |
| No | `ReadProgramData` | (Program, *Buffer, Size) - Read output of the program into a buffer. |
| No | `ReadProgramError` | (Program [, Format]) - Read a line from the standard error output of the program. |
| No | `ReadProgramString` | (Program [, Format]) - Read a line from the standard output of the program. |
| No | `RemoveEnvironmentVariable` | (Name$) - Remove the given variable from the environment. |
| No | `RunProgram` | (Filename$ [, Parameter$, WorkingDirectory$ [, Flags [, SenderProgram]]]) - Launch an external program. |
| No | `SetEnvironmentVariable` | (Name$, Value$) - Assign the given Value$ to the environment variable. |
| No | `WaitProgram` | (Program [, Timeout]) - Wait for the specified program to end. |
| No | `WriteProgramData` | (Program, *Buffer, Size) - Write data to the standard input of the program. |
| No | `WriteProgramString` | (Program, String$ [, Format]) - Write the string to the standard input of the program. |
| No | `WriteProgramStringN` | (Program, String$ [, Format]) - Write the string to the standard input of the program including a newline. |

## RegularExpression

0/19 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CountRegularExpressionGroups` | (#RegularExpression) - Return the number of capturing groups in the regular expression. |
| No | `CreateRegularExpression` | (#RegularExpression, Pattern$ [, Flags]) - Create a new regular expression. |
| No | `ExamineRegularExpression` | (#RegularExpression, String$) - Examine each match of the regular expression individually. |
| No | `ExtractRegularExpression` | (#RegularExpression, String$, Array$()) - Extract strings according to the regular expression into an array. |
| No | `FreeRegularExpression` | (#RegularExpression) - Free the specified #RegularExpression. |
| No | `IsRegularExpression` | (#RegularExpression) - Test if the given '#RegularExpression' is an initialized regular expression object. |
| No | `MatchRegularExpression` | (#RegularExpression, String$) - Test if a string matches the regular expression. |
| No | `NextRegularExpressionMatch` | (#RegularExpression) - Move to the next match of the regular expression. |
| No | `RegularExpressionError` | () - Return the last regular expression error. |
| No | `RegularExpressionGroup` | (#RegularExpression, Group) - Get the content of a matching group. |
| No | `RegularExpressionGroupLength` | (#RegularExpression, Group) - Get the length of a matching group. |
| No | `RegularExpressionGroupPosition` | (#RegularExpression, Group) - Get the position of a matching group within the current match. |
| No | `RegularExpressionMatchLength` | (#RegularExpression) - Get the length of the current regular expression match. |
| No | `RegularExpressionMatchPosition` | (#RegularExpression) - Get the position in the string of the current regular expression match. |
| No | `RegularExpressionMatchString` | (#RegularExpression) - Get the matching string from the current regular expression match. |
| No | `RegularExpressionNamedGroup` | (#RegularExpression, GroupName$) - Get the content of a matching named group. |
| No | `RegularExpressionNamedGroupLength` | (#RegularExpression, GroupName$) - Get the length of a matching named group. |
| No | `RegularExpressionNamedGroupPosition` | (#RegularExpression, GroupName$) - Get the position of a matching named group within the current match. |
| No | `ReplaceRegularExpression` | (#RegularExpression, String$, ReplaceString$) - Replace some strings according to the regular expression. |

## Requester

13/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `ColorRequester` | ([Color [, ParentID]]) - Open the standard requester to choose a color. |
| Yes | `FontRequester` | (FontName$, FontSize, Flags [, Color, [Style [, ParentID]]]) - Open the standard requester to choose a font. |
| Yes | `InputRequester` | (Title$, Message$, DefaultText$ [, Flags [, ParentID]]) - Open an input requester. |
| Yes | `MessageRequester` | (Title$, Text$ [, Flags [, ParentID]]) - Open a blocking requester to display some information. |
| Yes | `NextSelectedFilename` | () - Get the next selected filename. |
| Yes | `OpenFileRequester` | (Title$, DefaultFile$, Pattern$, PatternPosition [, Flags [, ParentID]]) - Open the standard requester which allows to choose a file. |
| Yes | `PathRequester` | (Title$, InitialPath$ [, ParentID]) - Open the standard path requester which allows to select a path. |
| Yes | `SaveFileRequester` | (Title$, DefaultFile$, Pattern$, PatternPosition [, ParentID]) - Open the standard requester which allows to save a file. |
| Yes | `SelectedFilePattern` | () - Return the selected pattern index chosen with OpenFileRequester() or SaveFileRequester(). |
| Yes | `SelectedFontColor` | () - Return the RGB color of the font chosen by the user with the FontRequester() command. |
| Yes | `SelectedFontName` | () - Return the name of the font chosen by the user with the FontRequester() command. |
| Yes | `SelectedFontSize` | () - Return the size of the font chosen by the user with the FontRequester() command. |
| Yes | `SelectedFontStyle` | () - Return the style of the font chosen by the user with the FontRequester() command. |

## Runtime

0/7 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `GetRuntimeDouble` | (Name) - Return the runtime double item value. |
| No | `GetRuntimeInteger` | (Name) - Return the runtime integer item value. |
| No | `GetRuntimeString` | (Name) - Return the runtime string item value. |
| No | `IsRuntime` | (Name) - Check if the specified item is available at runtime. |
| No | `SetRuntimeDouble` | (Name, Value) - Change the runtime item value. |
| No | `SetRuntimeInteger` | (Name, Value) - Change the runtime item value. |
| No | `SetRuntimeString` | (Name, String$) - Change the runtime item string. |

## Scintilla

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ScintillaGadget` | (#Gadget, x, y, Width, Height, Callback) - Create a new Scintilla gadget. |
| No | `ScintillaSendMessage` | (#Gadget, Message [, Param [, lParam]]) - Send a message to the Scintilla control. |

## Screen

0/19 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ChangeGamma` | (RedIntensity, GreenIntensity, BlueIntensity) - Change the Gamma for the current screen. |
| No | `ClearScreen` | (Color) - Clear the whole screen with the specified RGB color. |
| No | `CloseScreen` | () - Close the current screen (Either windowed or full screen mode). |
| No | `ExamineScreenModes` | () - Examine all the available screen modes. |
| No | `FlipBuffers` | () - Flip the back and front buffers on the current screen. |
| No | `IsScreenActive` | () - If the return is 0, then the screen is no longer active, else the screen is active. |
| No | `NextScreenMode` | () - Get the next screen mode information. |
| No | `OpenScreen` | (Width, Height, Depth, Title$ [, FlipMode [, RefreshRate]]) - Open a new screen according to the specified 'Width', 'Height' and 'Depth'. |
| No | `OpenWindowedScreen` | (WindowID(), x, y, Width, Height [, AutoStretch, RightOffset, BottomOffset [, FlipMode]]) |
| No | `ScreenDepth` | () - Get the screen depth. |
| No | `ScreenHeight` | () - Get the screen height. |
| No | `ScreenID` | () |
| No | `ScreenModeDepth` | () - Get the screen mode depth. |
| No | `ScreenModeHeight` | () - Get the screen mode height. |
| No | `ScreenModeRefreshRate` | () - Get the screen mode refresh rate. |
| No | `ScreenModeWidth` | () - Get the screen mode width. |
| No | `ScreenOutput` | () - Return the OutputID of the currently used screen to perform 2D rendering operations on it. |
| No | `ScreenWidth` | () - Get the screen width. |
| No | `SetFrameRate` | (FrameRate) - Set the frame rate (in frames per second) for the current screen. |

## ScreenGadget

0/54 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddScreenGadgetItem` | (#ScreenGadget, Position, Text$) - Adds an item to a gadget (ListView, ComboBox etc.) |
| No | `ButtonImageScreenGadget` | (#ScreenGadget, x, y, Width, Height, ImageID [, Flags]) - Creates a button image gadget in the current screen or screen window. |
| No | `ButtonScreenGadget` | (#ScreenGadget, x, y, Width, Height, Text$ [, Flags]) - Creates a button gadget in the current screen or screen window. |
| No | `CheckBoxScreenGadget` | (#ScreenGadget, x, y, Width, Height, Text$ [, Flags]) - Creates a checkbox gadget in the current screen or screen window. |
| No | `ClearScreenGadgetItems` | (#ScreenGadget) - Clears all the items from the specified #ScreenGadget. |
| No | `CloseScreenGadgetList` | () - Closes the current screen gadget list and go back to the previous one. |
| No | `CloseScreenWindow` | (#ScreenWindow) - Closes the specified screen window. |
| No | `ComboBoxScreenGadget` | (#ScreenGadget, x, y, Width, Height [, Flags]) - Creates a combobox gadget in the current screen or screen window. |
| No | `ContainerScreenGadget` | (#ScreenGadget, x, y, Width, Height, [, Flags]) - Creates a container gadget in the current screen or screen window. |
| No | `CountScreenGadgetItems` | (#ScreenGadget) - Returns the number of items in the specified #ScreenGadget. |
| No | `DisableScreenGadget` | (#ScreenGadget, State) - Disables or enables the #ScreenGadget. |
| No | `DisableScreenWindow` | (#ScreenWindow, State) - Disable or enable all user input in the window. |
| No | `EditorScreenGadget` | (#ScreenGadget, x, y, Width, Height [, Flags]) - Creates an editor gadget in the current screen or screen window. |
| No | `EventScreenGadget` | () - Returns the screen gadget number (#ScreenGagdet) which triggered the event. |
| No | `FreeScreenGadget` | (#ScreenGadget) - Frees the specified screen gadget |
| No | `GetScreenGadgetState` | (#ScreenGadget) - Gets the state of the #ScreenGadget. |
| No | `GetScreenGadgetText` | (#ScreenGadget) - Gets the text of the #ScreenGadget. |
| No | `HideScreenGadget` | (#ScreenGadget, State) - Hides or shows the specified screen gadget. |
| No | `HideScreenWindow` | (#ScreenWindow, State) - Hide or show the specified #ScreenWindow. |
| No | `ImageScreenGadget` | (#ScreenGadget, x, y, Width, Height, ImageID [, Flags]) - Creates a image gadget in the current screen or screen window. |
| No | `InitScreenGadgets` | () - Intialize the screen gadget library for the current screen. |
| No | `IsScreenGadget` | (#ScreenGadget) - Tests if the given screen gadget is a valid screen gadget object. |
| No | `ListViewScreenGadget` | (#ScreenGadget, x, y, Width, Height [, Flags]) - Creates a listview gadget in the current screen or screen window. |
| No | `OpenScreenGadgetList` | (#ScreenGadget [, GadgetItem]) - Makes the #ScreenGadget the current screen gadget list. |
| No | `OpenScreenWindow` | (#ScreenWindow, x, y, Width, Height [,Title$ [, Flags]]) - Open a new window on the current screen. |
| No | `OptionScreenGadget` | (#ScreenGadget, x, y, Width, Height, Text$) - Creates an option gadget in the current screen or screen window. |
| No | `PanelScreenGadget` | (#ScreenGadget, x, y, Width, Height) - Create a panel gadget in the current screen or screen window. |
| No | `ProgressBarScreenGadget` | (#ScreenGadget, x, y, Width, Height, Min, Max [, Flags]) - Creates a progressbar gadget in the current screen or screen window. |
| No | `RemoveScreenGadgetItem` | (#ScreenGadget, Position) - Removes the item at the specified position from the #ScreenGadget. |
| No | `RenderScreenGadgets` | () - Renders the screen window and gadgets on the current screen. |
| No | `ResizeScreenGadget` | (#ScreenGadget, x, y, Width, Height) - Moves and resizes the specified #ScreenGadget to the given position and dimensions. |
| No | `ResizeScreenWindow` | (#ScreenWindow, x, y, Width, Height) - Moves and resizes the specified #ScreenWindow to the given position and dimensions. |
| No | `ScreenEventType` | () - After ScreenWindowEvent(), use this function to determine of which type the event is. |
| No | `ScreenGadgetHeight` | (#ScreenGadget) - Returns the height of the #ScreenGadget, in pixels. |
| No | `ScreenGadgetSkin` | (SkinPart, #Image [, FreePrevious]) - Set an image for a specific skin part. |
| No | `ScreenGadgetsOption` | (Option, Value) - Set screen gadget options. |
| No | `ScreenGadgetToolTip` | (#ScreenGadget, Text$) - Associate the specified Text$ to the #ScreenGadget. |
| No | `ScreenGadgetType` | (#ScreenGadget) - Returns the type of the specified #ScreenGadget. |
| No | `ScreenGadgetWidth` | (#ScreenGadget) - Returns the width of the #ScreenGadget, in pixels. |
| No | `ScreenGadgetX` | (#ScreenGadget) - Returns the X position of the #ScreenGadget, in pixels. |
| No | `ScreenGadgetY` | (#ScreenGadget) - Returns the Y position of the #ScreenGadget, in pixels. |
| No | `ScreenMouseVisible` | (State) - Show or Hide the mouse. |
| No | `ScreenVirtualSize` | (VirtualWidth,VirtuelHeight) - Define VirtualWidth and VirtuelHeight to automaticly adjust gadget to screen resolution. |
| No | `ScreenWindowAnimation` | (#ScreenWindow, Way, Duration [, Fading [, Zoom [,x [,y [, Rotation]]]]) - Setup an animation to open the window. |
| No | `ScreenWindowEvent` | () - Check if an event has occurred on any of the opened screen windows. |
| No | `ScrollAreaScreenGadget` | (#ScreenGadget, x, y, Width, Height, ScrollAreaWidth, ScrollAreaHeight, ScrollStep [, Flags]) - Creates a scrollarea gadget in the current screen or screen window. |
| No | `ScrollBarScreenGadget` | (#ScreenGadget, x, y, Width, Height, Min, Max, PageLength [, Flags]) - Creates a scrollbar gadget in the current screen or screen window. |
| No | `SetScreenGadgetFont` | (#ScreenGadget, FontID) - Changes the font which will be used by newly created gadgets. |
| No | `SetScreenGadgetState` | (#ScreenGadget, State) - Changes the state of the #ScreenGadget. |
| No | `SetScreenGadgetText` | (#ScreenGadget, Text$) - Changes the text of the #ScreenGadget. |
| No | `SplitterScreenGadget` | (#ScreenGadget, x, y, Width, Height, #ScreenGadget1, #ScreenGadget2 [, Flags]) - Creates a splitter gadget in the current screen or screen window. |
| No | `StringScreenGadget` | (#ScreenGadget, x, y, Width, Height, Text$ [, Flags]) - Creates a string gadget in the current screen or screen window. |
| No | `TextScreenGadget` | (#ScreenGadget, x, y, Width, Height, Text$ [, Flags]) - Creates a text gadget in the current screen or screen window. |
| No | `TrackBarScreenGadget` | (#ScreenGadget, x, y, Width, Height, Min, Max [, Flags]) - Creates a trackbar gadget in the current screen or screen window. |

## SerialPort

0/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AvailableSerialPortInput` | (#SerialPort) - Return the number of remaining bytes in the input buffer. |
| No | `AvailableSerialPortOutput` | (#SerialPort) - Return the number of remaining bytes in the output buffer. |
| No | `CloseSerialPort` | (#SerialPort) - Close the specified #SerialPort. |
| No | `GetSerialPortStatus` | (#SerialPort, Attribute) - Return the specified status. |
| No | `IsSerialPort` | (#SerialPort) - Test if the given '#SerialPort' is an initialized SerialPort object. |
| No | `OpenSerialPort` | (#SerialPort, SerialPortName$, Bauds, Parity, Data, Stop, HandshakeMode, InputBufferSize, OutputBufferSize) - Open a serial port for use. |
| No | `ReadSerialPortData` | (#SerialPort, *Buffer, Length) - Read an arbitrary amount of data from the #SerialPort. |
| No | `SerialPortError` | (#SerialPort) - Return the current error on the #SerialPort, if any. |
| No | `SerialPortID` | (#SerialPort) - Return the unique ID which identifies the SerialPort in the operating system. |
| No | `SerialPortTimeouts` | (#SerialPort, RIT, RTTC, RTTM, WTTC, WTTM) - Change the default timeouts. |
| No | `SetSerialPortStatus` | (#SerialPort, Attribute, Value) - Change the #SerialPort status. |
| No | `WriteSerialPortData` | (#SerialPort, *Buffer, Length) - Write an arbitrary amount of data to the #SerialPort. |
| No | `WriteSerialPortString` | (#SerialPort, String$ [, Format]) - Write a string to the #SerialPort. |

## Skeleton

0/6 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddSkeletonAnimationKeyFrame` | (#Mesh, AnimationName$, Bone$, Time, RotationX, RotationY, RotationZ, RotationW, Mode [, TranslateX, TranslateY, TranslateZ [, ScaleX, ScaleY, ScaleZ]]) - Add a new keyframe to the skeleton animation. |
| No | `CreateBone` | (#Mesh, Bone$, ParentBone$, x, y, z, RotationX, RotationY, RotationZ, RotationW, Mode) - Create a new bone for the #Mesh. |
| No | `CreateSkeleton` | (#Mesh) - Create a new skeleton for the #Mesh. |
| No | `CreateSkeletonAnimation` | (#Mesh, AnimationName$, Length) - Create a new skeleton animation for the #Mesh. |
| No | `FinishBoneAssignment` | (#Mesh, SubMeshIndex) - Finish the bone assignment. |
| No | `VertexBoneAssignment` | (#Mesh, SubMeshIndex, VertexIndex, BoneIndex, Weight.f) - Assign a vertex to a bone. |

## Sort

0/9 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CustomSortArray` | (Array(), @CompareProcedure() [, Options [, Start, End]]) - Sort the specified array using a custom compare procedure. |
| No | `CustomSortList` | (List(), @CompareProcedure() [, Options [, Start, End]]) - Sort the specified list using a custom compare procedure. |
| No | `RandomizeArray` | (Array() [, Start, End]) - Reorder the items of the specified array in a random order. |
| No | `RandomizeList` | (List() [, Start, End]) - Reorder the items of the specified list in a random order. |
| No | `SortArray` | (Array(), Options [, Start, End]) - Sort the specified array according to the given options and range. |
| No | `SortList` | (List(), Options [, Start, End]) - Sort the specified list according to the given options and range. |
| No | `SortStructuredArray` | (Array(), Options, Offset, Type [, Start, End]) - Sort the specified array according to the given options and range. |
| No | `SortStructuredList` | (List(), Options, Offset, Type [, Start, End]) - Sort the specified list according to the given options and range. |
| No | `TypeOf` | (Object) - Returns the type of the given object. |

## Sound

0/19 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CatchSound` | (#Sound, *Buffer [, Size [, Flags]]) - Load a sound found at the specified memory address. |
| No | `EncodeSound` | (#Sound [, SoundPlugin [, Quality]]) - Encode the specified sound into memory. |
| No | `FreeSound` | (#Sound) - Stop and remove a sound from memory. |
| No | `GetSoundFrequency` | (#Sound [, Channel]) - Get the current #Sound frequency. |
| No | `GetSoundPosition` | (#Sound [, Mode [, Channel]]) - Get the current playing #Sound position. |
| No | `InitSound` | ([NbMaxChannels]) - Initialize the sound environment. If it returns zero then no sound can be played. |
| No | `IsSound` | (#Sound) - Test if the given '#Sound' is an initialized sound object. |
| No | `LoadSound` | (#Sound, FileName$ [, Flags]) - Load the specified sound into memory. |
| No | `PauseSound` | (#Sound [, Channel]) - Pause the specified sound (if it was playing). |
| No | `PlaySound` | (#Sound [, Flags [, Volume.f]]) - Start to play the #Sound. |
| No | `ResumeSound` | (#Sound [, Channel]) - Resume the play of the specified sound (if it was paused). |
| No | `SaveSound` | (#Sound, FileName$ [, SoundPlugin [, Quality]]) - Save the specified sound. |
| No | `SetSoundFrequency` | (#Sound, Frequency [, Channel]) - Set the new frequency for the #Sound. Values are from 1000 to 100000 hz. |
| No | `SetSoundPosition` | (#Sound, Position [, Mode [, Channel]]) - Set the current playing position for the #Sound. |
| No | `SoundLength` | (#Sound [, Mode]) - Return the length of the #Sound. |
| No | `SoundPan` | (#Sound, Pan [, Channel]) - Set the new pan for the #Sound. Values are from -100 to 100. |
| No | `SoundStatus` | (#Sound [, Channel]) - Get the specified #Sound status. |
| No | `SoundVolume` | (#Sound, Volume.f [, Channel]) - Set the new volume for the #Sound. Values are from 0 to 100. |
| No | `StopSound` | (#Sound [, Channel]) - Stop the specified sound (if it was playing). |

## Sound3D

0/10 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `FreeSound3D` | (#Sound3D) - Stop and remove a sound from memory. |
| No | `IsSound3D` | (#Sound3D) - Test if the given '#Sound3D' is an initialized sound object. |
| No | `LoadSound3D` | (#Sound3D, FileName$ [, Flags]) - Load the specified sound into memory. |
| No | `PlaySound3D` | (#Sound3D [, Flags]) - Start to play the #Sound3D. |
| No | `SoundCone3D` | (#Sound3D, InnerCone, OuterCode, OuterGain) - Specify the sound cone. |
| No | `SoundID3D` | (#Sound3D) - Return the unique system identifier for the sound. |
| No | `SoundListenerLocate` | (x, y, z) - Change the sound listener position in the world. |
| No | `SoundRange3D` | (#Sound3D, Minimum, Maximum) - Set the range for the #Sound3D. |
| No | `SoundVolume3D` | (#Sound3D, Volume) - Set the new volume for the #Sound3D. Values are from 0 to 100. |
| No | `StopSound3D` | (#Sound3D) - Stop the specified sound (if it was playing). |

## SoundPluginFLAC

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseFLACSoundDecoder` | () - Enable the FLAC support for CatchSound(), LoadSound(). |
| No | `UseFLACSoundEncoder` | () - Enable the FLAC support for SaveSound(), EncodeSound(). |

## SoundPluginMP3

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseMP3SoundDecoder` | () - Enable the MP3 support for CatchSound(), LoadSound(). |
| No | `UseMP3SoundEncoder` | ([LibraryName$]) - Enable the MP3 support for SaveSound(), EncodeSound(). |

## SoundPluginOGG

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseOGGSoundDecoder` | () - Enable the OGG support for CatchSound(), LoadSound(). |
| No | `UseOGGSoundEncoder` | () - Enable the OGG support for SaveSound(), EncodeSound(). |

## SoundPluginOpus

0/2 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `UseOpusSoundDecoder` | () - Enable the Opus support for CatchSound(), LoadSound(). |
| No | `UseOpusSoundEncoder` | () - Enable the Opus support for SaveSound(), EncodeSound(). |

## SpecialEffect

0/12 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AttachRibbonEffect` | (#Effect, NodeID) - Attach the ribbon #Effect to the node. |
| No | `CompositorEffectParameter` | (#Effect, TechniqueID, PassID, EffectName$, DataType, *Data) |
| No | `CreateCompositorEffect` | (#Effect, CameraID, EffectName$) |
| No | `CreateLensFlareEffect` | (#Effect, CameraID, NodeID, BurstSize, HaloSize, HaloMaterialID,	CircleMaterialID, BurstMaterialID) - Create a new lens flare #Effect. |
| No | `CreateRibbonEffect` | (#Effect, MaterialID, NbChains, NbElements, Length) |
| No | `DetachRibbonEffect` | (#Effect, NodeID) - Detach the ribbon #Effect from the node. |
| No | `FreeEffect` | (#Effect) - Free the specified #Effect. |
| No | `HideEffect` | (#Effect, State) |
| No | `IsEffect` | (#Effect) - Test if the given '#Effect' is an initialized effect object. |
| No | `LensFlareEffectColor` | (#Effect, ColorType, Color) - Change the color of the specified lens flare part. |
| No | `RibbonEffectColor` | (#Effect, ChainIndex, Color, FadeoutColor) |
| No | `RibbonEffectWidth` | (#Effect, ChainIndex, Width, FadeoutWidth) |

## Spline

0/13 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddSplinePoint` | (#Spline, x, y, z) |
| No | `ClearSpline` | (#Spline) - Clear the #Spline. |
| No | `ComputeSpline` | (#Spline, Time) - Compute the #Spline at the given time. |
| No | `CountSplinePoints` | (#Spline) - Return the number of points in the #Spline. |
| No | `CreateSpline` | (#Spline) - Create a new empty #Spline. |
| No | `FreeSpline` | (#Spline) - Free the #Spline. |
| No | `SplinePointX` | (#Spline, Index) - Return the x position of the #Spline point. |
| No | `SplinePointY` | (#Spline, Index) - Return the y position of the #Spline point. |
| No | `SplinePointZ` | (#Spline, Index) - Return the z position of the #Spline point. |
| No | `SplineX` | (#Spline) - Return the x position of the #Spline. |
| No | `SplineY` | (#Spline) - Return the y position of the #Spline. |
| No | `SplineZ` | (#Spline) - Return the z position of the #Spline. |
| No | `UpdateSplinePoint` | (#Spline, Index, x, y, z) - Update the specified #Spline point. |

## Sprite

0/25 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CatchSprite` | (#Sprite, MemoryID [, Mode]) - Catch a sprite from memory in BMP format. |
| No | `ClipSprite` | (#Sprite, x, y, Width, Height) - Add a clip zone to the specified sprite. |
| No | `CopySprite` | (#Sprite1, #Sprite2 [, Mode]) - Copy the #Sprite1 to #Sprite2. |
| No | `CreateSprite` | (#Sprite, Width, Height [, Mode]) - Create a new blank sprite. |
| No | `DisplaySprite` | (#Sprite, x.f, y.f) - Display the #Sprite at the specified position on the current sprite buffer. |
| No | `DisplayTransparentSprite` | (#Sprite, x.f, y.f [, Intensity [, Color]]) - Display the #Sprite at the specified position on the current screen. |
| No | `FreeSprite` | (#Sprite) - Remove the specified sprite from memory. |
| No | `GrabSprite` | (#Sprite, x, y, Width, Height [, Mode]) - Grab the screen content at the area x,y,Width,Height, and create a new #Sprite. |
| No | `InitSprite` | () - Init all the sprite environment for later use. |
| No | `IsSprite` | (#Sprite) - Test if the given '#Sprite' is an initialized sprite object. |
| No | `LoadSprite` | (#Sprite, FileName$ [, Mode]) - Load the specified sprite into the memory for immediate use. |
| No | `RotateSprite` | (#Sprite, Angle.f, Mode) - Rotate the sprite. Angle varies from 0 to 360. |
| No | `SaveSprite` | (#Sprite, Filename$ [, Format [, Flags]]) - Save a #Sprite on file in various image formats. |
| No | `SpriteBlendingMode` | (SourceMode, DestinationMode) - Change the realtime blending mode. |
| No | `SpriteCollision` | (#Sprite1, x1, y1, #Sprite2, x2, y2) - Test if the two sprites are overlapped. |
| No | `SpriteDepth` | (#Sprite) - Return the depth of the specified sprite. |
| No | `SpriteHeight` | (#Sprite) - Return the height in pixels of the specified sprite. |
| No | `SpriteID` | (#Sprite) - Return the unique ID which identifies the sprite in the operating system. |
| No | `SpriteOutput` | (#Sprite) - Return the output to the sprite, to be used with StartDrawing(). |
| No | `SpritePixelCollision` | (#Sprite1, x1, y1, #Sprite2, x2, y2) - Test if the two sprites are overlapped. |
| No | `SpriteQuality` | (Quality) - Set the level of rendering. 0=Lowest quality, 1=Bilinear filtering. |
| No | `SpriteWidth` | (#Sprite) - Return the width in pixels of the specified sprite. |
| No | `TransformSprite` | (#Sprite, x1, y1 [, z1], x2, y2 [, z2], x3, y3 [, z3], x4, y4 [, z4]) - Transform the sprite to the new given dimensions. |
| No | `TransparentSpriteColor` | (#Sprite, Color) - Change the sprite transparent color (when displayed with DisplayTransparentSprite). |
| No | `ZoomSprite` | (#Sprite, Width, Height) - Zoom the sprite to the new given dimensions. |

## StaticGeometry

0/5 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddStaticGeometryEntity` | (#StaticGeometry, EntityID, x, y, z [, ScaleX, ScaleY, ScaleZ [, RotateX, RotateY, RotateZ [, RotateW, Mode]]]) |
| No | `BuildStaticGeometry` | (#StaticGeometry) |
| No | `CreateStaticGeometry` | (#StaticGeometry, Width, Height, Length, EnableShadows) |
| No | `FreeStaticGeometry` | (#StaticGeometry) - Free the specified #StaticGeometry. |
| No | `IsStaticGeometry` | (#StaticGeometry) - Test if the given '#StaticGeometry' is an initialized static geometry object. |

## StatusBar

7/9 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `AddStatusBarField` | (Width) - Add a field to the current Status Bar previously created with CreateStatusBar(). |
| Yes | `CreateStatusBar` | (#StatusBar, WindowID) - Create and add an empty StatusBar to the specified WindowID. |
| Yes | `FreeStatusBar` | (#StatusBar) - Free the given status bar. |
| Yes | `IsStatusBar` | (#StatusBar) - Test if the given '#StatusBar' is an initialized statusbar object. |
| Yes | `StatusBarHeight` | (#StatusBar) - Return the height of the statusbar. |
| Yes | `StatusBarID` | (#StatusBar) - Return the unique ID which identifies the statusbar in the operating system. |
| No | `StatusBarImage` | (#StatusBar, Field, ImageID [, Appearance]) - Set an image to display in the specified statusbar field. |
| No | `StatusBarProgress` | (#StatusBar, Field, Value [, Appearance [, Min, Max]]) - Set the specified 'Field' to display a progressbar. |
| Yes | `StatusBarText` | (#StatusBar, Field, Text$ [, Appearance]) - Change the text of the specified 'Field' in the given '#StatusBar'. |

## String

15/37 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `Asc` | (String) - Return the character value of the first character in the string. |
| No | `Ascii` | (String$) - Create an Ascii string buffer. |
| No | `Bin` | (Number.q [, Type]) - Convert a number in binary string form. |
| Yes | `Chr` | (Value) - Return the character associated with the given unicode numeric value. |
| No | `CountString` | (String$, StringToCount$) - Count the number of occurrences of a string. |
| No | `EscapeString` | (String [, Flags]) - Return the escaped string. |
| No | `FindString` | (String$, StringToFind$ [, StartPosition [, Mode]]) - Return the Position of StringToFind$. |
| No | `FormatNumber` | (Number.d [, NbDecimals [, DecimalPoint$ [, ThousandSeperator$]]]) - Return the formatted string. |
| No | `Hex` | (Number [, Type]) - Convert a numeric number into a string, in hexadecimal format. |
| No | `InsertString` | (String$, StringToInsert$, Position) - Insert a string at the given position. |
| Yes | `LCase` | (String$) - Return the original string converted into lower case characters. |
| Yes | `Left` | (String$, Length) - Return the first 'Length' number of characters from the left side of the string. |
| Yes | `Len` | (String$) - Return the character length of the string. |
| No | `LSet` | (String$, Length [, Character]) - Pad a string to the left, and add 'space' characters to fit the specified length. |
| Yes | `LTrim` | (String$ [, Character$]) - Remove all the 'space' or given characters located at the beginning of a string. |
| Yes | `Mid` | (String$, StartPosition [, Length]) - Extract a string at the specified position from the given 'String$'. |
| No | `RemoveString` | (String$, RemoveString$ [, Mode [, StartPosition [, NbOccurrences]]]) - Find occurrences of 'RemoveString$' in the given 'String$' and remove them. |
| No | `ReplaceString` | (String$, StringToFind$, StringToReplace$ [, Mode [, StartPosition [, NbOccurrences]]]) - Replace occurrences of 'StringToFind$' with 'StringToReplace$' in the given 'String$'. |
| No | `ReverseString` | (String$) - Reverse all the characters of the string. |
| Yes | `Right` | (String$, Length) - Return the last 'Length' number of characters from the right side of the string. |
| No | `RSet` | (String$, Length [, Character]) - Pad a string to the right, and add 'space' characters to fit the specified length. |
| Yes | `RTrim` | (String$ [, Character$]) - Remove all the 'space' or given characters located at the end of a string. |
| No | `Space` | (Length) - Create an empty string of the given 'Length' filled with 'space' characters. |
| Yes | `Str` | (Value) - Convert a signed integer number into a string. |
| No | `StrD` | (Value [, NbDecimals]) - Convert a double number into a string. |
| Yes | `StrF` | (Value [, NbDecimals]) - Convert a float number into a string. |
| No | `StringByteLength` | (String$ [, Format]) - Get the length in bytes needed to store the string in the given format. |
| No | `StringField` | (String$, Index, Separator$) - Return the string field at the specified index. |
| No | `StrU` | (Value [, Type]) - Convert a value into unsigned form. |
| Yes | `Trim` | (String$ [, Character$]) - Remove all the 'space' or given characters located at the beginning and at the end of a string. |
| Yes | `UCase` | (String$) - Return the original string converted into upper case characters. |
| No | `UnescapeString` | (String [, Flags]) - Return the unescaped string. |
| No | `Unicode` | (String$) - Create an Unicode string buffer. |
| No | `UTF8` | (String$) - Create an UTF8 string buffer. |
| Yes | `Val` | (String$) - Transform a string into a numeric value. |
| No | `ValD` | (String$) - Convert a String into a double number. |
| Yes | `ValF` | (String$) - Convert a String into a float number. |

## StringBuilder

0/7 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AppendStringBuilderString` | (#StringBuilder, String$) - Append a string at the end of the current string. |
| No | `AppendStringBuilderStringN` | (#StringBuilder, String$) - Append a string at the end of the current string and add a newline. |
| No | `CreateStringBuilder` | (#StringBuilder [, IncrementLength]) - Create a new string builder. |
| No | `FreeStringBuilder` | (#StringBuilder) - Free the string builder. |
| No | `GetStringBuilderString` | (#StringBuilder) - Get the current built string. |
| No | `IsStringBuilder` | (#StringBuilder) - Check if the given '#StringBuilder' is an initialized string builder object. |
| No | `ResetStringBuilder` | (#StringBuilder) - Reset the string builder to an empty string. |

## System

2/9 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `ComputerName` | () - Return the name of the computer. |
| No | `CountCPUs` | ([Type]) - Return the number of processors available. |
| No | `CPUName` | () - Return the name of the CPU. |
| Yes | `Delay` | (Time) - Wait for the specified time (in milliseconds). |
| Yes | `ElapsedMilliseconds` | () - Return the number of milliseconds that have elapsed since a specific time in the past. |
| No | `MemoryStatus` | (Type) - Return information about the available memory in the computer. |
| No | `OSLanguage` | () - Return the ISO 639 language code the OS is currently running in. |
| No | `OSVersion` | () - Retrieve the OS version number. |
| No | `UserName` | () - Return the name of the currently logged in user. |

## SystemGtk

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `DoubleClickTime` | () - Return the system setting for the double-click time in milliseconds. |

## SysTray

6/6 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `AddSysTrayIcon` | (#SysTrayIcon, WindowID, ImageID) - Add an icon in the SysTray area. |
| Yes | `ChangeSysTrayIcon` | (#SysTrayIcon, ImageID) - Change the specified icon in the SysTray area. |
| Yes | `IsSysTrayIcon` | (#SysTrayIcon) - Test if the given '#SysTrayIcon' is an initialized systray icon object. |
| Yes | `RemoveSysTrayIcon` | (#SysTrayIcon) - Remove the icon from the SysTray area. |
| Yes | `SysTrayIconMenu` | (#SysTrayIcon, MenuID) - Associate a popup menu to the specified icon. |
| Yes | `SysTrayIconToolTip` | (#SysTrayIcon, Text$) - Associate a tooltip text to the specified icon. |

## Terrain

0/24 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddTerrainTexture` | (#Terrain, Layer, WorldSize, DiffuseSpecular$, NormalHeight$) - Add a texture to the #Terrain. |
| No | `BuildTerrain` | (#Terrain) - Construct the #Terrain. |
| No | `CreateTerrain` | (#Terrain, Size, WorldSize, Scale, NbLayers, Filename$, Extension$) - Create a new #Terrain. |
| No | `CreateTerrainBody` | (#Terrain, Restitution, Friction) - Add a static physic body to the #Terrain. |
| No | `DefineTerrainTile` | (#Terrain, TileX, TileY, HeightMap$, FlipX, FlipY) - Define a new #Terrain tile. |
| No | `FreeTerrain` | (#Terrain) - Free the #Terrain. |
| No | `FreeTerrainBody` | (#Terrain) - Free the body associated with the #Terrain. |
| No | `GetTerrainTileHeightAtPoint` | (#Terrain, TileX, TileY, x, y) - Get the height of the #Terrain tile at the specified position. |
| No | `GetTerrainTileLayerBlend` | (#Terrain, TileX, TileY, Layer, x, y) - Get the #Terrain tile layer blend value. |
| No | `SaveTerrain` | (#Terrain, ModifiedOnly) - Save the #Terrain on disk. |
| No | `SetTerrainTileHeightAtPoint` | (#Terrain, TileX, TileY, x, y, Height) - Set the height of the #Terrain tile at the specified position. |
| No | `SetTerrainTileLayerBlend` | (#Terrain, TileX, TileY, Layer, x, y, Value) - Set the #Terrain tile layer blend value. |
| No | `SetupTerrains` | (LightID, CompositeMapDistance, Flags) - Setup the default options for all terrains. |
| No | `TerrainHeight` | (#Terrain, x, z) - Return the height of the #Terrain at the specified coordinates. |
| No | `TerrainLocate` | (#Terrain, x, y, z) - Change the #Terrain location. |
| No | `TerrainMousePick` | (#Terrain, CameraID, x, y) - Perform a mouse picking on the #Terrain. |
| No | `TerrainRenderMode` | (#Terrain, Flags) - Change the #Terrain render mode. |
| No | `TerrainTileHeightAtPosition` | (#Terrain, TileX, TileY, Layer, x, y) - Return the height of the #Terrain tile. |
| No | `TerrainTileLayerMapSize` | (#Terrain, TileX, TileY) - Return the #Terrain tile layer map size. |
| No | `TerrainTilePointX` | (#Terrain, TileX, TileY, x, y, z) - Return the x position in the #Terrain tile of the point. |
| No | `TerrainTilePointY` | (#Terrain, TileX, TileY, x, y, z) - Return the y position in the #Terrain tile of the point. |
| No | `TerrainTileSize` | (#Terrain, TileX, TileY) - Return the size of the #Terrain tile. |
| No | `UpdateTerrain` | (#Terrain) - Update the whole #Terrain. |
| No | `UpdateTerrainTileLayerBlend` | (#Terrain, TileX, TileY, Layer) - Update the #Terrain tile blend value. |

## Text3D

0/12 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CreateText3D` | (#Text3D, Caption$ [, Font$, Scale, Color]) - Create a new 3D text. |
| No | `FreeText3D` | (#Text3D) - Free the specified #Text3D. |
| No | `IsText3D` | (#Text3D) - Test if the given '#Text3D' is an initialized text3d object. |
| No | `MoveText3D` | (#Text3D, x, y, z [, Mode]) - Move the #Text3D. |
| No | `ScaleText3D` | (#Text3D, x, y, z [, Mode]) - Scale the #Text3D. |
| No | `Text3DAlignment` | (#Text3D, Alignment) - Change the displayed text alignment. |
| No | `Text3DCaption` | (#Text3D, Caption$) - Change the displayed text. |
| No | `Text3DColor` | (#Text3D, Color) - Change the displayed text color. |
| No | `Text3DID` | (#Text3D) - Return the unique #Text3D ID. |
| No | `Text3DX` | (#Text3D) - Return the x position of the #Text3D. |
| No | `Text3DY` | (#Text3D) - Return the y position of the #Text3D. |
| No | `Text3DZ` | (#Text3D) - Return the z position of the #Text3D. |

## Texture

0/16 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CopyTexture` | (#Texture, #NewTexture) - Copy the specified #Texture on the #NewTexture. |
| No | `CreateCubeMapTexture` | (#Texture, Width, Height, TextureName$ [, BackColor]) - Create a new cube map #Texture. |
| No | `CreateCubicTexture` | (#Texture, #Texture1, #Texture2, #Texture3, #Texture4, #Texture5, #Texture6) - Create a new cubic #Texture. |
| No | `CreateRenderTexture` | (#Texture, CameraID, Width, Height [, Flags [, RenderTextureName$]]) - Create a new render #Texture. |
| No | `CreateTexture` | (#Texture, Width, Height [, Name$]) - Create a new blank #Texture. |
| No | `EntityCubeMapTexture` | (#Texture, #Entity) - Apply the cube map #Texture to the #Entity. |
| No | `FreeTexture` | (#Texture) - Free the specified #Texture. |
| No | `GetScriptTexture` | (#Texture, Name$) - Get the specified #Texture as defined in the script file. |
| No | `IsTexture` | (#Texture) - Test if the given '#Texture' is an initialized texture object. |
| No | `LoadTexture` | (#Texture, Filename$) - Load the specified #Texture. |
| No | `SaveRenderTexture` | (#Texture, Filename$) - Save the specified #Texture. |
| No | `TextureHeight` | (#Texture) - Return the height in pixels of the #Texture. |
| No | `TextureID` | (#Texture) - Return the #Texture unique ID. |
| No | `TextureOutput` | (#Texture) - Return the OutputID of the #Texture to perform a 2D rendering operation on it. |
| No | `TextureWidth` | (#Texture) - Return the width in pixels of the #Texture. |
| No | `UpdateRenderTexture` | (#Texture) - Update the #Texture content with the camera view. |

## Thread

17/18 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `CreateMutex` | () - Create a new mutex synchronization object. |
| Yes | `CreateSemaphore` | ([InitialCount]) - Create a new semaphore synchronization object. |
| Yes | `CreateThread` | (@Procedure(), Parameter) - Create a new thread running in the application background. |
| Yes | `FreeMutex` | (Mutex) - Free the mutex object. |
| Yes | `FreeSemaphore` | (Semaphore) - Free the semaphore object. |
| Yes | `IsThread` | (Thread) - Check if the thread exists. |
| Yes | `KillThread` | (Thread) - Kill the thread. Warning, this is a very dangerous function. |
| Yes | `LockMutex` | (Mutex) - Wait for ownership of the mutex object. |
| Yes | `PauseThread` | (Thread) - Pause the thread execution. |
| Yes | `ResumeThread` | (Thread) - Resume the thread execution. |
| Yes | `SignalSemaphore` | (Semaphore) - Signal the semaphore object once (increment the count by 1). |
| Yes | `ThreadID` | (Thread) - Get the OS handle for the thread. |
| No | `ThreadPriority` | (Thread, Priority) - Change the thread priority and return the old priority. |
| Yes | `TryLockMutex` | (Mutex) - Try to get ownership of the mutex object. Return 0 if unsuccessful. |
| Yes | `TrySemaphore` | (Semaphore) - Decrement the semaphore count only if it is above 0, else return 0. |
| Yes | `UnlockMutex` | (Mutex) - Release ownership of the mutex object. |
| Yes | `WaitSemaphore` | (Semaphore) - Wait for the semaphore object once (decrement the count by 1). |
| Yes | `WaitThread` | (Thread [, Timeout]) - Wait until the thread ends. |

## ToolBar

12/12 implemented.

| Status | Procedure | Signature |
|---|---|---|
| Yes | `CreateToolBar` | (#ToolBar, WindowID [, Flags]) - Create an empty toolbar identified by the number '#ToolBar'. |
| Yes | `DisableToolBarButton` | (#ToolBar, ButtonID, State) - Disable or enable the given toolbar button (1=Disable, 0=Enable). |
| Yes | `FreeToolBar` | (#ToolBar) - Free the specified #ToolBar. |
| Yes | `GetToolBarButtonState` | (#ToolBar, ButtonID) - Get the state of the specified button. |
| Yes | `IsToolBar` | (#ToolBar) - Test if the given '#ToolBar' is an initialized toolbar object. |
| Yes | `SetToolBarButtonState` | (#ToolBar, ButtonID, State) - Change the state of the specified button. |
| Yes | `ToolBarButtonText` | (#ToolBar, ButtonID, Text$) - Change the toolbar button text$. |
| Yes | `ToolBarHeight` | (#ToolBar) - Return the height of the specified #ToolBar. |
| Yes | `ToolBarID` | (#ToolBar) - Return the unique ID which identifies the toolbar in the operating system. |
| Yes | `ToolBarImageButton` | (#ButtonID, ImageID [, Mode [, Text$]]) - Add an image button to the toolbar being constructed. |
| Yes | `ToolBarSeparator` | () - Add a vertical separator to the toolbar being constructed. |
| Yes | `ToolBarToolTip` | (#ToolBar, ButtonID, Text$) - Associate the specified Text$ to the current ToolBar #ButtonID. |

## VectorDrawing

0/66 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddPathArc` | (x1.d, y1.d, x2.d, y2.d, Radius.d, [, Flags]) - Add an arc to the vector drawing path. |
| No | `AddPathBox` | (x.d, y.d, Width.d, Height.d [, Flags]) - Add a box to the vector drawing path. |
| No | `AddPathCircle` | (x.d, y.d, Radius.d [, StartAngle.d, EndAngle.d [, Flags]]) - Add a circle (or part of it) to the vector drawing path. |
| No | `AddPathCurve` | (x1.d, y1.d, x2.d, y2.d, x3.d, y3.d [, Flags]) - Add a curve to the vector drawing path. |
| No | `AddPathEllipse` | (x.d, y.d, RadiusX.d, RadiusY.d [, StartAngle.d, EndAngle.d [, Flags]]) - Add an ellipse (or part of it) to the vector drawing path. |
| No | `AddPathLine` | (x.d, y.d [, Flags]) - Add a straight line to the vector drawing path. |
| No | `AddPathSegments` | (Segments$ [, Flags]) - Add path segments described in a string to the path. |
| No | `AddPathText` | (Text$) - Add text to the vector drawing path. |
| No | `BeginVectorLayer` | ([Alpha]) - Begin a new transparent layer for vector drawing. |
| No | `ClipPath` | ([Flags]) - Add the vector drawing path to the output clipping. |
| No | `ClosePath` | () - Close the current vector drawing path by connecting start & end with a straight line. |
| No | `ConvertCoordinateX` | (x.d, y.d [, Source, Target]) - Convert a point between coordinate systems. |
| No | `ConvertCoordinateY` | (x.d, y.d [, Source, Target]) - Convert a point between coordinate systems. |
| No | `CustomDashPath` | (Width.d, Array.d() [, Flags [, StartOffset.d]]) - Draw a dashed line with custom dash lengths along the vector drawing path. |
| No | `DashPath` | (Width.d, Length.d [, Flags [, StartOffset.d]]) - Draw an evenly dashed line along the vector drawing path. |
| No | `DotPath` | (Width.d, Distance.d [, Flags [, StartOffset.d]]) - Draw a dotted line along the vector drawing path. |
| No | `DrawVectorImage` | (ImageID [, Alpha [, Width.d, Height.d]]) - Draw an image to the vector output. |
| No | `DrawVectorParagraph` | (Text$, Width.d, Height.d [, Flags]) - Draw a paragraph of text inside a bounding box. |
| No | `DrawVectorText` | (Text$) - Draw text directly to the vector drawing output. |
| No | `EndVectorLayer` | () - End a transparent layer for vector drawing. |
| No | `FillPath` | ([Flags]) - Fill the area defined by the vector drawing path. |
| No | `FillVectorOutput` | () - Fill the entire vector drawing output with the drawing source. |
| No | `FlipCoordinatesX` | (AxisX.d [, System]) - Flip the X coordinates of the vector drawing coordinate system along an axis. |
| No | `FlipCoordinatesY` | (AxisY.d [, System]) - Flip the Y coordinates of the vector drawing coordinate system along an axis. |
| No | `IsInsidePath` | (x.d, y.d [, CoordinateSystem [, Flags]]) - Test whether the given point is inside the area of the current path. |
| No | `IsInsideStroke` | (x.d, y.d, Width.d [, Flags [, CoordinateSystem]]) - Test whether the given point is inside the area stroked by the current path. |
| No | `IsPathEmpty` | () - Return true if the current vector drawing path is empty. |
| No | `MovePathCursor` | (x.d, y.d [, Flags]) - Move the vector drawing cursor without adding a line. |
| No | `NewVectorPage` | () - Begin a new page in the vector drawing output. |
| No | `PathBoundsHeight` | () - Get the height of the bounding box of the current path. |
| No | `PathBoundsWidth` | () - Get the width of the bounding box of the current path. |
| No | `PathBoundsX` | () - Get the X coordinate of the bounding box of the current path. |
| No | `PathBoundsY` | () - Get the Y coordinate of the bounding box of the current path. |
| No | `PathCursorX` | () - Get the current X coordinate of the path cursor. |
| No | `PathCursorY` | () - Get the current Y coordinate of the path cursor. |
| No | `PathLength` | () - Get the length of the path. |
| No | `PathPointAngle` | (Distance.d) - Get the angle of the path at the given distance from the start. |
| No | `PathPointX` | (Distance.d) - Get the X coordinate of the path point at the given distance from the start. |
| No | `PathPointY` | (Distance.d) - Get the Y coordinate of the path point at the given distance from the start. |
| No | `PathSegments` | () - Get a description of the current path as a string. |
| No | `PdfVectorOutput` | (Filename$, Width.d, Height.d [, Unit]) - Create a vector drawing output for a PDF file. |
| No | `ResetCoordinates` | ([System]) - Reset the vector drawing coordinate system to the default. |
| No | `ResetPath` | () - Clear the current vector drawing path. |
| No | `RestoreVectorState` | () - Restore the vector drawing state to the saved state. |
| No | `RotateCoordinates` | (x.d, y.d, Angle.d [, System]) - Rotate the vector drawing coordinate system. |
| No | `SaveVectorState` | () - Save the current vector drawing state. |
| No | `ScaleCoordinates` | (ScaleX.d, ScaleY.d [, System]) - Scale the vector drawing coordinate system. |
| No | `SkewCoordinates` | (AngleX.d, AngleY.d [, System]) - Skew the vector drawing coordinate system. |
| No | `StartVectorDrawing` | (Output) - Start vector drawing on the specified output. |
| No | `StopVectorDrawing` | () - Stop vector drawing on the current output. |
| No | `StrokePath` | (Width.d [, Flags]) - Draw a line along the vector drawing path. |
| No | `TranslateCoordinates` | (x.d, y.d [, System]) - Move the origin of the vector drawing coordinate system. |
| No | `VectorFont` | (FontID [, Size.d]) - Set the font to use for vector drawing. |
| No | `VectorOutputHeight` | () - Return the height of the current vector output. |
| No | `VectorOutputWidth` | () - Return the width of the current vector output. |
| No | `VectorParagraphHeight` | (Text$, Width.d, Height.d) - Calculate the height of a text paragraph for vector drawing. |
| No | `VectorResolutionX` | () - Return horizontal resolution (in DPI) of the vector drawing output. |
| No | `VectorResolutionY` | () - Return vertical resolution (in DPI) of the vector drawing output. |
| No | `VectorSourceCircularGradient` | (x.d, y.d, Radius.d, [CenterX.d, CenterY.d]) - Set a circular gradient for vector drawing. |
| No | `VectorSourceColor` | (Color) - Set the color for vector drawing. |
| No | `VectorSourceGradientColor` | (Color, Position.d) - Add a color to the vector drawing gradient. |
| No | `VectorSourceImage` | (ImageID [, Alpha [, Width.d, Height.d [, Flags]]]) - Set an image as the vector drawing source. |
| No | `VectorSourceLinearGradient` | (x1.d, y1.d, x2.d, y2.d) - Set a linear gradient for vector drawing. |
| No | `VectorTextHeight` | (Text$ [, Flags]) - Calculate the text height for vector drawing. |
| No | `VectorTextWidth` | (Text$ [, Flags]) - Calculate the text width for vector drawing. |
| No | `VectorUnit` | () - Return the unit of measurement used by the vector drawing output. |

## VectorDrawingSvg

0/1 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `SvgVectorOutput` | (Filename$, Width.d, Height.d [, Unit]) - Create a vector drawing output for an SVG file. |

## Vehicle

0/8 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddVehicleWheel` | (#Entity, #WheelEntity, xCP.f, yCP.f, zCP.f, AxisX.f, AxisY.f, AxisZ.f, MaxSuspensionLength.f, WheelRadius.f, TractionWheel, RollInfluence.f) - Add a wheel to a vehicle. |
| No | `ApplyVehicleBrake` | (#Entity, Wheel, Brake.f) - Apply brakes force to a vehicle wheel. |
| No | `ApplyVehicleForce` | (#Entity, Wheel, Force.f) - Apply force to a vehicle wheel. |
| No | `ApplyVehicleSteering` | (#Entity, Wheel, Steering.f) - Apply steering to a vehicle wheel. |
| No | `CreateVehicle` | (#Entity) - Create a new vehicle entity. |
| No | `CreateVehicleBody` | (#Entity, Mass.f, Restitution.f, Friction.f [, SuspensionStiffness.f, SuspensionCompression.f, SuspensionDamping.f, MaxSuspensionCompression.f, Friction.f]) - Create a body for a vehicle. |
| No | `GetVehicleAttribute` | (#Entity, Attribute, Wheel) - Get a vehicle attribute. |
| No | `SetVehicleAttribute` | (#Entity, Attribute, Value.f [, Wheel]) - Set a vehicle attribute. |

## VertexAnimation

0/8 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddVertexPoseReference` | (#Mesh, Animation$, Track, Index, PoseIndex, Influence) - Add a new vertex pose reference to the #Mesh track. |
| No | `CreateVertexAnimation` | (#Mesh, Animation$, Length) - Create a new vertex animation for the #Mesh. |
| No | `CreateVertexPoseKeyFrame` | (#Mesh, Animation$, Track, Time) - Create a new keyframe to the #Mesh track. |
| No | `CreateVertexTrack` | (#Mesh, Animation$, Index) - Create a new track to the #Mesh animation. |
| No | `MeshPoseCount` | (#Mesh) - Return the mesh pose count. |
| No | `MeshPoseName` | (#Mesh, PoseIndex) - Return the mesh pose name. |
| No | `UpdateVertexPoseReference` | (#Mesh, Animation$, Track, Index, PoseIndex, Influence) - Update a vertex pose reference in the #Mesh track. |
| No | `VertexPoseReferenceCount` | (#Mesh, Animation$, Track, Index) - Return the vertex pose reference size of the specified #Mesh track. |

## WebView

0/5 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `BindWebViewCallback` | (#Gadget, JavaScriptFunction$, @Callback()) - Bind a callback to a JavaScript function. |
| No | `UnbindWebViewCallback` | (#Gadget, JavaScriptFunction$) - Unbind a callback from the WebView. |
| No | `WebViewExecuteScript` | (#Gadget, JavaScript$) - Execute asynchronously a script in the WebView. |
| No | `WebViewGadget` | (#Gadget, x, y, Width, Height [, Flags]) - Create a new WebView gadget. |
| No | `WebViewProxy` | (URL$, Port [Username$, Password$]) - Set up the web proxy to use for the WebViewGadget() |

## Window

8/40 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AddKeyboardShortcut` | (#Window, Key, Event) - Add a keyboard shortcut to the specified window. |
| No | `AddWindowTimer` | (#Window, Timer, Timeout) - Add a timer event to the window. |
| Yes | `CloseWindow` | (#Window) - Close the specified window. |
| No | `DisableWindow` | (#Window, State) - Disable or enable all user input to the window. |
| No | `GetActiveWindow` | () - Return the currently active window (the one with the focus). |
| No | `GetWindowColor` | (#Window) - Get the background color of the window. |
| No | `GetWindowData` | (#Window) - Return the value previously stored for the window. |
| No | `GetWindowState` | (#Window) - Get the minimized/maximized state of the window. |
| No | `GetWindowTitle` | (#Window) - Get the current window title. |
| Yes | `HideWindow` | (#Window, State [, Flags]) - Hide or show the specified window. |
| Yes | `IsWindow` | (#Window) - Test if the given window is an initialized window object. |
| Yes | `OpenWindow` | (#Window, x, y, InnerWidth, InnerHeight, Title$ [, Flags [, ParentID]]) - Open a new window according to the specified parameters. |
| No | `RemoveKeyboardShortcut` | (#Window, Key) - Remove a keyboard shortcut from the specified window. |
| No | `RemoveWindowTimer` | (#Window, Timer) - Remove the timer event from the window. |
| Yes | `ResizeWindow` | (#Window, x, y, Width, Height) - Move and resize the specified window to the given position and dimensions. |
| No | `SetActiveWindow` | (#Window) - Activate the specified window. This means that focus has been put on this window. |
| No | `SetWindowColor` | (#Window, Color) - Set the background color of the window. |
| No | `SetWindowData` | (#Window, Value) - Store a value for the window. |
| No | `SetWindowState` | (#Window, State) - Set the minimized/maximized state of the window. |
| No | `SetWindowTitle` | (#Window, Title$) - Change the current window title. |
| No | `SmartWindowRefresh` | (#Window, State) - Activate or deactivate the smart window refresh. |
| No | `StickyWindow` | (#Window, State) - Make the window stay in front of all other windows. |
| Yes | `WaitWindowEvent` | ([Timeout]) - Wait until an event occurs on any of the opened windows. |
| No | `WindowBounds` | (#Window, MinimumWidth, MinimumHeight, MaximumWidth, MaximumHeight) - Set the window bounds. |
| Yes | `WindowEvent` | () - Check if an event has occurred on any of the opened windows. |
| No | `WindowHeight` | (#Window [, Flags]) - Return the height, in pixels, of the specified window. |
| Yes | `WindowID` | (#Window) - Return the unique ID which identifies the window in the operating system. |
| No | `WindowMouseX` | (#Window) - Return the mouse position relative to the left of the specified window. |
| No | `WindowMouseY` | (#Window) - Return the mouse position relative to the top of the specified window. |
| No | `WindowOutput` | (#Window) - Return the OutputID of the specified window to perform 2D rendering operations on it. |
| No | `WindowResolutionX` | (#Window) - Return the DPI resolution scale factor of the screen the specified window is on, on the 'x' axis (1.0 if no scaling is applied). |
| No | `WindowResolutionY` | (#Window) - Return the DPI resolution scale factor of the screen the specified window is on, on the 'y' axis (1.0 if no scaling is applied). |
| No | `WindowScaledX` | (#Window, x) - Scale a value on the 'x' axis according to the DPI of the screen the specified window is on. |
| No | `WindowScaledY` | (#Window, y) - Scale a value on the 'y' axis according to the DPI of the screen the specified window is on. |
| No | `WindowUnscaledX` | (#Window, x) - Unscale a value on the 'x' axis according to the DPI of the screen the specified window is on. |
| No | `WindowUnscaledY` | (#Window, y) - Unscale a value on the 'y' axis according to the DPI of the screen the specified window is on. |
| No | `WindowVectorOutput` | (#Window [, Unit]) - Return the OutputID of the specified window to perform vector drawing on it. |
| No | `WindowWidth` | (#Window [, Flags]) - Return the width, in pixels, of the specified window. |
| No | `WindowX` | (#Window [, Mode]) - Return the left position, in pixels, of the specified window. |
| No | `WindowY` | (#Window [, Mode]) - Return the top position, in pixels, of the specified window. |

## World

0/36 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `AmbientColor` | (Color) - Change the ambient color of the world. |
| No | `BodyPick` | (#Camera, Picked, x, y, Locked) - Allow a body to be manipulated with the mouse. |
| No | `CreateWater` | (WaveTextureID, FoamTextureID, WaterColor, SkyColor, Distance.f, WaveBig.f, WaveSmall.f, Swell.f, Foam.f) - Create a water plane at the given position. |
| No | `EnableWorldCollisions` | (Enable) - Enable the world collisions. |
| No | `EnableWorldPhysics` | (Enable) - Enable the world physics. |
| No | `Engine3DStatus` | (Type) - Get the state of the 3D engine. |
| No | `ExamineWorldCollisions` | (Contacts) - Start to examine the occurred collisions in the world. |
| No | `FirstWorldCollisionEntity` | () - Return the first entity of the current collision. |
| No | `Fog` | (Color, Intensity, StartDistance, EndDistance) - Display a fog effect. |
| No | `GetWorldAttribute` | (Attribute) - Get the specified shadow attribute. |
| No | `MousePick` | (#Camera, x, y [, PickMask]) - Return the first #Entity at the given position. |
| No | `MouseRayCast` | (#Camera, x, y, PickMask) - Return whether a ray has encountered an object. |
| No | `NextWorldCollision` | () - Get the next world collision. |
| No | `NormalX` | () - Return the last pick normal X value. |
| No | `NormalY` | () - Return the last pick normal Y value. |
| No | `NormalZ` | () - Return the last pick normal Z value. |
| No | `PickX` | () - Return the last pick X position. |
| No | `PickY` | () - Return the last pick Y position. |
| No | `PickZ` | () - Return the last pick Z position. |
| No | `PointPick` | (#Camera, x, y) - Get the direction of the given point. |
| No | `RayCast` | (x, y, z, DestinationX, DestinationY, DestinationZ, PickMask) - Return #True if the ray has crossed an object. |
| No | `RayCollide` | (x, y, z, DestinationX, DestinationY, DestinationZ [, CollisionGroup, CollisionMask]) - Check if the given ray collides with an object. |
| No | `RayPick` | (x, y, z, DestinationX, DestinationY, DestinationZ [, PickMask]) - Return the first #Entity crossed by the ray. |
| No | `RenderWorld` | ([ElapsedPhysicTime]) - Render the 3D world on the screen. |
| No | `SecondWorldCollisionEntity` | () - Return the second entity of the current collision. |
| No | `SetWorldAttribute` | (Attribute, Value) - Set the specified shadow attribute. |
| No | `ShowGUI` | (Intensity, ShowMouseCursor [, #Camera, Enable]) - Show or hide the GUI. |
| No | `SkyBox` | (Filename$ [, FogColor, FogIntensity, FogStartDistance, FogEndDistance]) - Create a 6 faces textured cube to simulate a closed world. |
| No | `SkyDome` | (CloudTextureID, SkyColor, RiseColor [, NbCloudLayers, , CloudsHeight.f, CloudsSpeedX.f, CloudsSpeedZ.f]) - Create an artificial sky dome. |
| No | `WaterHeight` | (#Camera, x, y) - Get the Y position of the water at the given coordinates. |
| No | `WorldCollisionAppliedImpulse` | () - Get applied impulse vector of the current collision. |
| No | `WorldCollisionContact` | () - Get the contact vector of the current collision. |
| No | `WorldCollisionNormal` | () - Get the normal vector of the current collision. |
| No | `WorldDebug` | (Mode) - Change the debug mode for the whole world. |
| No | `WorldGravity` | (Gravity [, x, y, z]) - Change the gravity in the world. |
| No | `WorldShadows` | (ShadowType [, Distance [, Color [, TextureSize]]]) - Change the shadow handling in this world. |

## Xml

0/57 implemented.

| Status | Procedure | Signature |
|---|---|---|
| No | `CatchXML` | (#XML, *Address, Size [, Flags [, Encoding]]) - Parse XML from memory. |
| No | `ChildXMLNode` | (Node [, n]) - Get the nth child of the given XML node (default is the 1st child). |
| No | `ComposeXML` | (#XML [, Flags]) - Compose an XML tree into a string. |
| No | `CopyXMLNode` | (Node, ParentNode [, PreviousNode]) - Copy the XML node to a new location (or different XML tree). |
| No | `CreateXML` | (#XML [, Encoding]) - Create a new empty XML tree. |
| No | `CreateXMLNode` | (ParentNode, Name$ [, PreviousNode [, Type]]) - Create a new node inside the XML tree. |
| No | `DeleteXMLNode` | (Node) - Delete the given XML node and all its children from its XML tree. |
| No | `ExamineXMLAttributes` | (Node) - Examine the attribute list of the given XML node. |
| No | `ExportXML` | (#XML, *Address, Size [, Flags]) - Export an XML tree to a memory buffer. |
| No | `ExportXMLSize` | (#XML [, Flags]) - Get the number of bytes required to export an XML tree. |
| No | `ExtractXMLArray` | (Node, Array() [, Flags]) - Extract an XML node into the specified Array(). |
| No | `ExtractXMLList` | (Node, List() [, Flags]) - Extract an XML node into the specified List(). |
| No | `ExtractXMLMap` | (Node, Map() [, Flags]) - Extract an XML node into the specified Map(). |
| No | `ExtractXMLStructure` | (Node, *Buffer, Structure [, Flags]) - Extract an XML node into the specified structure. |
| No | `FormatXML` | (#XML, Flags [, IndentStep]) - Clean up the XML for a better look. |
| No | `FreeXML` | (#XML) - Free an XML tree. |
| No | `GetXMLAttribute` | (Node, Attribute$) - Get the value of the given attribute of the XML node. |
| No | `GetXMLEncoding` | (#XML) - Get the encoding used in the parsed XML data. |
| No | `GetXMLNodeName` | (Node) - Get the (tag-)name of the given XML node. |
| No | `GetXMLNodeOffset` | (Node) - Get the number of characters of text between this node and the previous one in the tree. |
| No | `GetXMLNodeText` | (Node) - Get the text content of the given XML node. |
| No | `GetXMLStandalone` | (#XML) - Get the standalone mode of the XML document. |
| No | `InsertXMLArray` | (ParentNode, Array() [, PreviousNode]) - Insert the specified Array() into the XML node. |
| No | `InsertXMLList` | (ParentNode, List() [, PreviousNode]) - Insert the specified List() into the XML node. |
| No | `InsertXMLMap` | (ParentNode, Map() [, PreviousNode]) - Insert the specified Map() into the XML node. |
| No | `InsertXMLStructure` | (ParentNode, *Buffer, Structure [, PreviousNode]) - Insert the specified structure data into the XML node. |
| No | `IsXML` | (#XML) - Check if #XML represents a valid XML object. |
| No | `LoadXML` | (#XML, FileName$ [, Encoding]) - Load an XML file and parse it. |
| No | `MainXMLNode` | (#XML) - Get the main node of the XML tree. |
| No | `MoveXMLNode` | (Node, ParentNode [, PreviousNode]) - Move the XML node to a new location (or different XML tree). |
| No | `NextXMLAttribute` | (Node) - Get the next attribute in the examined XML node. |
| No | `NextXMLNode` | (Node) - Get the next XML node on the same level as the given node. |
| No | `ParentXMLNode` | (Node) - Get the parent of the given XML node. |
| No | `ParseXML` | (#XML, Input$) - Parse XML from a string. The XML must be encoded in PB string format. |
| No | `PreviousXMLNode` | (Node) - Get the previous XML node on the same level as the given node. |
| No | `RemoveXMLAttribute` | (Node, Attribute$) - Remove the attribute entry from the XML node. |
| No | `ResolveXMLAttributeName` | (Node, Attribute$ [, Separator$]) - Resolve all namespaces for the given node attribute. |
| No | `ResolveXMLNodeName` | (Node [, Separator$]) - Resolve all namespaces for the given node and return a full node name. |
| No | `RootXMLNode` | (#XML) - Get the root-node of the XML tree. |
| No | `SaveXML` | (#XML, FileName$ [, Flags]) - Save an XML tree to a file. |
| No | `SetXMLAttribute` | (Node, Attribute$, Value$) - Set the value of the given attribute of the XML node (creates the attribute if needed). |
| No | `SetXMLEncoding` | (#XML, Encoding) - Set the encoding used to export/save the XML tree. |
| No | `SetXMLNodeName` | (Node, Name$) - Set the (tag-)name of the given XML node. |
| No | `SetXMLNodeOffset` | (Node, Offset) - Set the number of characters of text between this node and the previous one in the tree. |
| No | `SetXMLNodeText` | (Node, Text$) - Set the text content of the given XML node. |
| No | `SetXMLStandalone` | (#XML, Standalone) - Set the standalone mode of the XML document. |
| No | `XMLAttributeName` | (Node) - Get the name of the current examined attribute in the XML node. |
| No | `XMLAttributeValue` | (Node) - Get the value of the current examined attribute in the XML node. |
| No | `XMLChildCount` | (Node) - Get the number of children that the given XML node has. |
| No | `XMLError` | (#XML) - Get a string description for an error during XML parsing. |
| No | `XMLErrorLine` | (#XML) - Get the input line where a parsing error occurred. |
| No | `XMLErrorPosition` | (#XML) - Get the input column where a parsing error occurred. |
| No | `XMLNodeFromID` | (#XML, ID$) - Get a node in the XML tree from its ID-tag. |
| No | `XMLNodeFromPath` | (ParentNode, Path$) - Get a node in the XML tree from a path string. |
| No | `XMLNodePath` | (Node [, ParentNode]) - Get the path string that represents the given XML node in the document (or relative to ParentNode). |
| No | `XMLNodeType` | (Node) - Get the type of the given XML node. |
| No | `XMLStatus` | (#XML) - Get the status (or error code) of the XML parsing. |

