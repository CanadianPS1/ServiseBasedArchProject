import services.auth_service.test_suite
import services.game_service.test_suite

import asyncio

if __name__ == "__main__":
    print("\nRunning Game Service tests...")
    asyncio.run(services.game_service.test_suite.main())
    print("\n\n\nRunning Auth Service tests...")
    asyncio.run(services.auth_service.test_suite.main())