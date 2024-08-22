This example database application exposes libmdbx with a subset of Bitcoin
Core's `dbwrapper` interface with the libmdbx key-value db engine.

It is part of an experiment to see if there are any DB engines that offer better
performance than leveldb for Bitcoin Core's coins database.
