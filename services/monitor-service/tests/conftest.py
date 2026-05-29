import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = [
    "pytest_userver.plugins.core",
    "pytest_userver.plugins.postgresql",
]


@pytest.fixture(scope="session")
def allowed_url_prefixes_extra():
    return [
        "http://localhost:8080/",
        "http://127.0.0.1:8080/",
    ]


@pytest.fixture(scope="session")
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        "monitor_service",
        [service_source_dir.joinpath("postgresql/migrations")],
    )
    return pgsql_local_create(list(databases.values()))
