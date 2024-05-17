This extension, `shellfs`, allow you to use Unix pipes for input and output in DuckDB on Unix and Mac OS X.

If you add a pipe character `|` to the name of the filename it will be assumed to be a collection of commands to run and the output will be captured by DuckDB.

These examples are quite simple, in real world use you'd likely be running another program that produces CSV, JSON or some other format to handle some complexity that DuckDB cannot handle natively.

### Examples of reading output into DuckDB

```sql
-- Really simple hello world, obviously not necessary.
SELECT content from read_text('printf "Hello World" |');
┌─────────────┐
│   content   │
│   varchar   │
├─────────────┤
│ Hello World │
└─────────────┘

-- Generate a sequence only return numbers that contain a 2
SELECT * from read_csv('seq 1 100 | grep 2 |');
┌─────────┐
│ column0 │
│  int64  │
├─────────┤
│       2 │
│      12 │
│      20 │
│      21 │
│      22 │
└─────────┘

-- Get the first multiples of 7 between 1 and 3 5
-- demonstrate how commands can be chained together
SELECT * from read_csv('seq 1 35 | awk "\$1 % 7 == 0" | head -n 2 |');
┌─────────┐
│ column0 │
│  int64  │
├─────────┤
│       7 │
│      14 │
└─────────┘

-- Do some arbitrary curl
SELECT abbreviation, unixtime from read_json('curl -s http://worldtimeapi.org/api/timezone/Etc/UTC  |');
┌──────────────┬────────────┐
│ abbreviation │  unixtime  │
│   varchar    │   int64    │
├──────────────┼────────────┤
│ UTC          │ 1715983565 │
└──────────────┴────────────┘
```

### Examples of writing output

```sql
-- Write all numbers from 1 to 30 out, but then filter via grep
-- for only lines that contain 6.
COPY (select * from unnest(generate_series(1, 30))) TO '| grep 6 > numbers.csv' (FORMAT 'CSV');
```


## Building

### Build steps
Now to build the extension, run:
```sh
make
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/shellfs/shellfs.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded.
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `shellfs.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. The template contains a single scalar function `quack()` that takes a string arguments and returns a string:
```
D select quack('Jane') as result;
┌───────────────┐
│    result     │
│    varchar    │
├───────────────┤
│ Quack Jane 🐥 │
└───────────────┘
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test
```

### Installing the deployed binaries
To install your extension binaries from S3, you will need to do two things. Firstly, DuckDB should be launched with the
`allow_unsigned_extensions` option set to true. How to set this will depend on the client you're using. Some examples:

CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url of your bucket + version of the extension
you want to install. To do this run the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/shellfs/latest';
```
Note that the `/latest` path will allow you to install the latest extension version available for your current version of
DuckDB. To specify a specific version, you can pass the version instead.

After running these steps, you can install and load your extension using the regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL shellfs
LOAD shellfs
```
