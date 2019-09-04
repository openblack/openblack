/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Game.h>
#include <SDL.h>
#include <iostream>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv)
{
	std::cout << "==============================================================================\n"
	             "   openblack - A modern reimplementation of Lionhead's Black & White (2001)   \n"
	             "==============================================================================\n"
	             "\n";

	try
	{
		std::unique_ptr<OpenBlack::Game> game(new OpenBlack::Game(argc, argv));
		game->Run();
	}
	catch (std::runtime_error& e)
	{
		// Only catch runtime_error as these should be user issues.
		// Catching other types would just make debugging them more difficult.

		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", e.what(), nullptr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	return main(__argc, __argv);
}
#endif
