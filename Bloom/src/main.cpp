import wire.core;

int main()
{
	wire::ApplicationDesc desc{};
	desc.WindowTitle = "bloom";
	desc.WindowWidth = 1280;
	desc.WindowHeight = 720;

	wire::Application app(desc);

	app.run();

	return 0;
}