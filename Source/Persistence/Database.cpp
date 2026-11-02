#include "Database.h"
#include <sqlite3.h>

namespace drummatch
{

Database::Database() = default;

Database::~Database()
{
    close();
}

juce::File Database::getDefaultDatabaseFile()
{
    const auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                                 .getChildFile("Application Support")
#endif
                                 .getChildFile("KitMatch");

    if (!appDataDir.isDirectory())
        appDataDir.createDirectory();

    auto dbFile = appDataDir.getChildFile("kitmatch.db");

    // Migration fallback: if kitmatch.db doesn't exist, check legacy drummatch.db
    if (!dbFile.existsAsFile())
    {
        const auto legacyDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                                     .getChildFile("Application Support")
#endif
                                     .getChildFile("DrumMatch");
        const auto legacyDb = legacyDir.getChildFile("drummatch.db");
        if (legacyDb.existsAsFile())
        {
            legacyDb.copyFileTo(dbFile);
        }
    }

    return dbFile;
}

bool Database::open(const juce::File& dbFile)
{
    const juce::ScopedLock sl(dbLock);
    close();

    dbFile.getParentDirectory().createDirectory();

    const int rc = sqlite3_open_v2(dbFile.getFullPathName().toRawUTF8(),
                                  &db,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                  nullptr);

    if (rc != SQLITE_OK)
    {
        close();
        return false;
    }

    // Activar WAL mode para velocidad y concurrencia
    execute("PRAGMA journal_mode = WAL;");
    execute("PRAGMA synchronous = NORMAL;");
    execute("PRAGMA foreign_keys = ON;");

    createTables();
    return true;
}

void Database::close()
{
    const juce::ScopedLock sl(dbLock);
    if (db != nullptr)
    {
        sqlite3_close_v2(db);
        db = nullptr;
    }
}

bool Database::execute(const char* sql)
{
    if (db == nullptr)
        return false;

    char* errMsg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        if (errMsg != nullptr)
            sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void Database::createTables()
{
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS libraries (
            id TEXT PRIMARY KEY,
            path TEXT NOT NULL,
            name TEXT NOT NULL,
            is_enabled INTEGER NOT NULL DEFAULT 1,
            sample_count INTEGER NOT NULL DEFAULT 0,
            last_scanned INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS samples (
            id TEXT PRIMARY KEY,
            library_name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            relative_path TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            last_modified INTEGER NOT NULL,
            sample_rate REAL NOT NULL,
            num_channels INTEGER NOT NULL,
            length_seconds REAL NOT NULL,
            bits_per_sample INTEGER NOT NULL,
            format TEXT NOT NULL,
            category INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_samples_lib ON samples(library_name);
        CREATE INDEX IF NOT EXISTS idx_samples_cat ON samples(category);

        CREATE TABLE IF NOT EXISTS pad_layouts (
            layout_id TEXT NOT NULL,
            pad_index INTEGER NOT NULL,
            pad_id TEXT NOT NULL,
            label TEXT NOT NULL,
            category INTEGER NOT NULL,
            color_argb INTEGER NOT NULL,
            assigned_file_path TEXT,
            match_score REAL DEFAULT 0.0,
            PRIMARY KEY (layout_id, pad_index)
        );

        CREATE TABLE IF NOT EXISTS app_state (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
    )";

    execute(schema);
    execute("ALTER TABLE pad_layouts ADD COLUMN match_score REAL DEFAULT 0.0;");
}

std::vector<LibraryFolder> Database::loadLibraries()
{
    const juce::ScopedLock sl(dbLock);
    std::vector<LibraryFolder> result;
    if (db == nullptr)
        return result;

    const char* sql = "SELECT id, path, name, is_enabled, sample_count, last_scanned FROM libraries ORDER BY name ASC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            LibraryFolder f;
            f.id = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            f.directory = juce::File(juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));
            f.name = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            f.isEnabled = sqlite3_column_int(stmt, 3) != 0;
            f.sampleCount = sqlite3_column_int(stmt, 4);
            f.lastScanned = juce::Time(sqlite3_column_int64(stmt, 5));
            result.push_back(f);
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::saveLibrary(const LibraryFolder& folder)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    const char* sql = R"(
        INSERT INTO libraries (id, path, name, is_enabled, sample_count, last_scanned)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            path = excluded.path,
            name = excluded.name,
            is_enabled = excluded.is_enabled,
            sample_count = excluded.sample_count,
            last_scanned = excluded.last_scanned;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, folder.id.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, folder.directory.getFullPathName().toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, folder.name.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, folder.isEnabled ? 1 : 0);
    sqlite3_bind_int(stmt, 5, folder.sampleCount);
    sqlite3_bind_int64(stmt, 6, folder.lastScanned.toMilliseconds());

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateLibraryEnabled(const juce::String& folderId, bool enabled)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    const char* sql = "UPDATE libraries SET is_enabled = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
    sqlite3_bind_text(stmt, 2, folderId.toRawUTF8(), -1, SQLITE_TRANSIENT);

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateLibraryCount(const juce::String& folderId, int count, juce::Time lastScanned)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    const char* sql = "UPDATE libraries SET sample_count = ?, last_scanned = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, count);
    sqlite3_bind_int64(stmt, 2, lastScanned.toMilliseconds());
    sqlite3_bind_text(stmt, 3, folderId.toRawUTF8(), -1, SQLITE_TRANSIENT);

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteLibrary(const juce::String& folderId)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    // Obtener nombre de la biblioteca para borrar sus samples
    juce::String libName;
    {
        const char* q = "SELECT name FROM libraries WHERE id = ?;";
        sqlite3_stmt* stmtQ = nullptr;
        if (sqlite3_prepare_v2(db, q, -1, &stmtQ, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmtQ, 1, folderId.toRawUTF8(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmtQ) == SQLITE_ROW)
                libName = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmtQ, 0)));
            sqlite3_finalize(stmtQ);
        }
    }

    execute("BEGIN TRANSACTION;");

    const char* sqlLib = "DELETE FROM libraries WHERE id = ?;";
    sqlite3_stmt* stmt1 = nullptr;
    if (sqlite3_prepare_v2(db, sqlLib, -1, &stmt1, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt1, 1, folderId.toRawUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt1);
        sqlite3_finalize(stmt1);
    }

    if (libName.isNotEmpty())
    {
        const char* sqlSamp = "DELETE FROM samples WHERE library_name = ?;";
        sqlite3_stmt* stmt2 = nullptr;
        if (sqlite3_prepare_v2(db, sqlSamp, -1, &stmt2, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt2, 1, libName.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt2);
            sqlite3_finalize(stmt2);
        }
    }

    execute("COMMIT;");
    return true;
}

std::vector<Sample> Database::loadAllSamples()
{
    const juce::ScopedLock sl(dbLock);
    std::vector<Sample> result;
    if (db == nullptr)
        return result;

    const char* sql = R"(
        SELECT id, library_name, file_path, relative_path, file_size,
               last_modified, sample_rate, num_channels, length_seconds,
               bits_per_sample, format, category
        FROM samples;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            Sample s;
            s.id = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            s.libraryName = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            s.file = juce::File(juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
            s.relativePath = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            s.fileSize = sqlite3_column_int64(stmt, 4);
            s.lastModified = juce::Time(sqlite3_column_int64(stmt, 5));
            s.sampleRate = sqlite3_column_double(stmt, 6);
            s.numChannels = sqlite3_column_int(stmt, 7);
            s.lengthInSeconds = sqlite3_column_double(stmt, 8);
            s.bitsPerSample = sqlite3_column_int(stmt, 9);
            s.format = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)));
            s.category = static_cast<InstrumentCategory>(sqlite3_column_int(stmt, 11));

            result.push_back(s);
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::saveSamplesBatch(const juce::String& libraryName, const std::vector<Sample>& samples)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    execute("BEGIN TRANSACTION;");

    // Borrar previos de esta biblioteca
    {
        const char* delSql = "DELETE FROM samples WHERE library_name = ?;";
        sqlite3_stmt* delStmt = nullptr;
        if (sqlite3_prepare_v2(db, delSql, -1, &delStmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(delStmt, 1, libraryName.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_step(delStmt);
            sqlite3_finalize(delStmt);
        }
    }

    // Insertar en lote
    const char* insSql = R"(
        INSERT INTO samples (id, library_name, file_path, relative_path, file_size,
                             last_modified, sample_rate, num_channels, length_seconds,
                             bits_per_sample, format, category)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* insStmt = nullptr;
    if (sqlite3_prepare_v2(db, insSql, -1, &insStmt, nullptr) == SQLITE_OK)
    {
        for (const auto& s : samples)
        {
            sqlite3_reset(insStmt);
            sqlite3_bind_text(insStmt, 1, s.id.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insStmt, 2, s.libraryName.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insStmt, 3, s.file.getFullPathName().toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insStmt, 4, s.relativePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(insStmt, 5, s.fileSize);
            sqlite3_bind_int64(insStmt, 6, s.lastModified.toMilliseconds());
            sqlite3_bind_double(insStmt, 7, s.sampleRate);
            sqlite3_bind_int(insStmt, 8, s.numChannels);
            sqlite3_bind_double(insStmt, 9, s.lengthInSeconds);
            sqlite3_bind_int(insStmt, 10, s.bitsPerSample);
            sqlite3_bind_text(insStmt, 11, s.format.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insStmt, 12, static_cast<int>(s.category));

            sqlite3_step(insStmt);
        }
        sqlite3_finalize(insStmt);
    }

    execute("COMMIT;");
    return true;
}

bool Database::deleteSamplesForLibrary(const juce::String& libraryName)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    const char* delSql = "DELETE FROM samples WHERE library_name = ?;";
    sqlite3_stmt* delStmt = nullptr;
    if (sqlite3_prepare_v2(db, delSql, -1, &delStmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(delStmt, 1, libraryName.toRawUTF8(), -1, SQLITE_TRANSIENT);
    const bool ok = (sqlite3_step(delStmt) == SQLITE_DONE);
    sqlite3_finalize(delStmt);
    return ok;
}

bool Database::updateSampleCategoriesBatch(const std::vector<std::pair<juce::String, InstrumentCategory>>& updates)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr || updates.empty())
        return false;

    execute("BEGIN TRANSACTION;");
    const char* sql = "UPDATE samples SET category = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        for (const auto& item : updates)
        {
            sqlite3_reset(stmt);
            sqlite3_bind_int(stmt, 1, static_cast<int>(item.second));
            sqlite3_bind_text(stmt, 2, item.first.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }
    execute("COMMIT;");
    return true;
}

std::vector<PadDefinition> Database::loadPadLayout(const juce::String& layoutId)
{
    const juce::ScopedLock sl(dbLock);
    std::vector<PadDefinition> pads;
    if (db == nullptr)
        return PadDefinition::getDefaultLayout();

    const char* sql = R"(
        SELECT pad_index, pad_id, label, category, color_argb, assigned_file_path, match_score
        FROM pad_layouts
        WHERE layout_id = ?
        ORDER BY pad_index ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, layoutId.toRawUTF8(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            PadDefinition pad;
            pad.index = sqlite3_column_int(stmt, 0);
            pad.padId = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            pad.label = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            pad.category = static_cast<InstrumentCategory>(sqlite3_column_int(stmt, 3));
            pad.color = juce::Colour(static_cast<juce::uint32>(sqlite3_column_int64(stmt, 4)));

            const char* assignedPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            if (assignedPath != nullptr)
            {
                juce::File assignedFile(juce::String::fromUTF8(assignedPath));
                if (assignedFile.existsAsFile())
                {
                    Sample s;
                    s.file = assignedFile;
                    s.id = juce::String::toHexString(assignedFile.getFullPathName().hashCode64());
                    s.category = pad.category;
                    pad.assignedSample = s;
                }
            }

            pad.matchScore = static_cast<float>(sqlite3_column_double(stmt, 6));
            pads.push_back(pad);
        }
        sqlite3_finalize(stmt);
    }

    if (pads.size() != 16)
        return PadDefinition::getDefaultLayout();

    // Preservar atajos de teclado por defecto
    const auto defaultLayout = PadDefinition::getDefaultLayout();
    for (size_t i = 0; i < pads.size() && i < defaultLayout.size(); ++i)
    {
        pads[i].keyShortcut = defaultLayout[i].keyShortcut;
    }

    return pads;
}

bool Database::savePadLayout(const juce::String& layoutId, const std::vector<PadDefinition>& pads)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return false;

    execute("BEGIN TRANSACTION;");

    const char* delSql = "DELETE FROM pad_layouts WHERE layout_id = ?;";
    sqlite3_stmt* delStmt = nullptr;
    if (sqlite3_prepare_v2(db, delSql, -1, &delStmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(delStmt, 1, layoutId.toRawUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_step(delStmt);
        sqlite3_finalize(delStmt);
    }

    const char* insSql = R"(
        INSERT INTO pad_layouts (layout_id, pad_index, pad_id, label, category, color_argb, assigned_file_path, match_score)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* insStmt = nullptr;
    if (sqlite3_prepare_v2(db, insSql, -1, &insStmt, nullptr) == SQLITE_OK)
    {
        for (const auto& p : pads)
        {
            sqlite3_reset(insStmt);
            sqlite3_bind_text(insStmt, 1, layoutId.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insStmt, 2, p.index);
            sqlite3_bind_text(insStmt, 3, p.padId.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insStmt, 4, p.label.toRawUTF8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insStmt, 5, static_cast<int>(p.category));
            sqlite3_bind_int64(insStmt, 6, static_cast<sqlite3_int64>(p.color.getARGB()));

            if (p.assignedSample.has_value() && p.assignedSample->file.existsAsFile())
            {
                sqlite3_bind_text(insStmt, 7, p.assignedSample->file.getFullPathName().toRawUTF8(), -1, SQLITE_TRANSIENT);
            }
            else
            {
                sqlite3_bind_null(insStmt, 7);
            }

            sqlite3_bind_double(insStmt, 8, static_cast<double>(p.matchScore));

            sqlite3_step(insStmt);
        }
        sqlite3_finalize(insStmt);
    }

    execute("COMMIT;");
    return true;
}

juce::String Database::getAppState(const juce::String& key, const juce::String& defaultValue)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return defaultValue;

    const char* sql = "SELECT value FROM app_state WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    juce::String result = defaultValue;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, key.toRawUTF8(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            result = juce::String::fromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

void Database::setAppState(const juce::String& key, const juce::String& value)
{
    const juce::ScopedLock sl(dbLock);
    if (db == nullptr)
        return;

    const char* sql = R"(
        INSERT INTO app_state (key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, key.toRawUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, value.toRawUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

} // namespace drummatch

