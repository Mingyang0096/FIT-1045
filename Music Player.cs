using static SplashKitSDK.SplashKit;
using static System.Convert;

WriteLine("Welcome to the music play test");
WriteLine();

Write("What file would you like to load? Enter path: ");
string file_path = ReadLine();
Write("What should I call this music? Enter name: ");
string music_name = ReadLine();

WriteLine();

LoadSoundEffect(music_name, file_path);
WriteLine($"Loading {music_name} music from {file_path}");

WriteLine();

Write("How long do you want to play it for? Enter seconds: ");
string period = ReadLine();
float periods = ToSingle(period);
Write("At what initial volume? Enter percent (0 to 1): ");
string a = ReadLine();
float volume1 = ToSingle(a);
Write("At what repeat volume? Enter percent (0 to 1): ");
string b = ReadLine();
float volume2 = ToSingle(b);

WriteLine();

int duration = ToInt32(period) * 1000;
WriteLine($"Playing {music_name} at {volume1} volume for {periods} seconds...");
PlaySoundEffect(music_name, volume1);
Delay(duration);
StopSoundEffect(music_name);
WriteLine("Stopping music");

WriteLine();

WriteLine($"Playing {music_name} at {volume2} volume for {periods} seconds...");
PlaySoundEffect(music_name, volume2);
Delay(duration);
StopSoundEffect(music_name);
WriteLine("Stopping music");
