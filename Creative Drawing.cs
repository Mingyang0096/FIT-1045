using static System.Convert;
using static SplashKitSDK.SplashKit;

Write("Welcom friend. What is your name: ");    // input name
string name = ReadLine();
WriteLine($"Hello {name}");
Write("What are you most scared of? ");
string item = ReadLine();
Write("Delay speed scale? [100 to 1000]: ");
string a = ReadLine();
int speed = ToInt32(a)*2;    // transition speed data type

OpenWindow("exloration", 800, 600);

ClearScreen(ColorBlack());
DrawText("You are approaching what looks a hunted house", ColorDarkRed(), 100, 200);
RefreshScreen();
FillRectangle(ColorGray(), 300, 300, 200, 200);
FillTriangle(ColorRed(), 250, 300, 400, 150, 550, 300);
RefreshScreen();      
Delay(speed);           //delay - scale by speed scale factor

// play door creak sound effect - delay scaled
LoadSoundEffect("door", "C:/door.ogg");
PlaySoundEffect("door");
Delay(speed);

ClearScreen(ColorBlack());
DrawText("It is  dark in here...", ColorDarkRed(), 100, 200);
DrawText($"{name}!!", ColorDarkRed(), 100, 200);
Delay(speed);
DrawText($"{item} everywhere!", ColorDarkRed(), 100, 200);
RefreshScreen();
CloseWindow("exloration");

// open window - show black - delay
OpenWindow("scinario2", 1200, 900);
ClearScreen(ColorBlack());
RefreshScreen();

// draw scary picture - play scream sound effect - short delay
LoadBitmap("scary", "C:/image.ogg");
DrawBitmap("scary", 200, 100);
LoadSoundEffect("scream", "C:/scary");
PlaySoundEffect("scream");
RefreshScreen();
Delay(speed);

// draw black screen - short delay
ClearScreen(ColorBlack());
DrawBitmap("scary", 200, 100);
RefreshScreen();
Delay(speed / 2);

// draw scary picture again - short delay
ClearScreen(ColorBlack());
DrawBitmap("scary", 200, 100);
RefreshScreen();
Delay(speed);

// draw black screen - short delay
ClearScreen(ColorBlack());
RefreshScreen();
Delay(speed);

// draw scary picture again - with "game" logo below and the text "coming soon"
DrawText("game",ColorBlue(), 600, 100);
DrawText("coming soon", ColorWhite(), 400, 300);
RefreshScreen();
Delay(speed);
