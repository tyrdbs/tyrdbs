**NOTE** This project moved to [source hut](https://git.sr.ht/~tyrtech/tyrdbs).

About
====

**tyrdbs** is a database system framework which provides users with the ability
to construct highly performant, low latency and scalable databases suited for
their specific needs. tyrdbs stands for tyr database system. *Týr* (/tɪər/; Old
Norse: Týr, pronounced [tyːr]) - Old Norse god of war.


User-centric
====

The tyrdbs is user-centric as the user is able to provide key components
which define how parts of the system behave. In turn, this enables
implementation of various database use-cases like timeseries databases, wide
column databases, or any other for that matter. These use-cases are able to
coexist within a single system and at the same time be highly optimized for each
specific one. Users are able to control how and where the data gets written,
how is the data merged and read.

Getting started
====

First, you'll need a few things:

1. gcc/clang supporting c++17 (with charconv)
2. scons >= 3.1
3. linux kernel >= 5.6
4. liburing >= 0.6
5. libuuid
6. git

After you've installed those on your system, checkout the code with:

```
# git clone https://github.com/tyrdbs/tyrdbs.git
```

Create a build directory:

```
# mkdir ./tyrdbs/build && cd ./tyrdbs/build
```

And run scons:

```
# scons -Qu -j4
```

After these steps, you should have quite a few compiled executables in tests
directory. Most have detailed descriptions of the command line interface. For
example:

```
# ./tests/ping_client --help
Usage: ./tests/ping_client [option] <uri>
Ping client.

Mandatory arguments to long options are mandatory for short options too.
      --help                display this help and exit
      --network-queue-depth=num
                            network queue depth to use (default is 512)
      --cpu=index           cpu index to run the program on (default is 0)
      --threads=num         number of threads to use (default is 32)
      --iterations=num      number of iterations per thread to do (default is
                              10000)
  <uri>                     uri to connect to
```

Architecture
====

The central point of the architecture is a micro-shard (μshard or ushard in the
code). The point of the micro-shard is to keep part of the data which forms a
collection. Collection is a named grouping of key-value pairs. Every collection
has the same number of micro-shards which is predetermined when the array is
first initialized and cannot be changed during the lifetime of the array. The
number of micro-shards is theoretically limited to 2^32 but practical
limitations restrict this number to a smaller value. Commonly seen numbers are
16k, 32k and 64k. A single micro-shard from all collections is kept at a single
data node which forms a non-divisible data unit. Multiple micro-shards can be
contained within a single data node. Having a large number of micro-shards helps
with the ability to distribute data easily across data nodes and keep a single
micro-shard's size relatively small which allows various maintenance processes
-- like merging -- to have relatively low impact on concurrent readers and
writers.

Micro-shard consists of a tiered lists of slices which form a LSM tree like
structure. Slice is a container holding key-value pairs. In its current form,
slice is a variation of B+ tree extended to to allow for fast key existence
testing for both point and range queries. Nodes within the tree are compressed
using the L4Z algorithm. Each slice is created by either merging of slices form
a specific tier or by a direct data stream which constitutes an update to the
collection's micro-shard. Slice's tier is determined by the number of key-value
pairs it holds after it has been fully written to. Slices are write-once
structures and, in effect, keys are never updated directly but rather
overwritten by keys in a newer slice. Determining which key is newer is done
with the help from an index counter which is kept per collection's micro-shard.
At this moment, keys are limited to 1024 bytes while values it can hold are not
limited at all. To allow for low memory overhead while handling values, values
are split internally based on available node space at the moment they are
written. Reconstruction happens client side, if needed, so reading keys from
tyrdbs is more like fetching a stream than fetching key-value pairs. This allows
a high degree of flexibility to the user as to how to handle its data. Within a
collection, keys can have variable sizes and are sorted using naturally byte
string orderings.

Having many micro-shards within a single process where every micro-shard has
many slices, results in having millions of slices. Having milion of slices
translates to having millions of files needed to be handled by a single process.
Merges and writes also create a dynamic process where slices are created and
destroyed at high rates. This creates a noticeable strain on the file system
and the kernel in general. To solve this issue, a user-land file system was
introduced. It supports write once in append only mode and reading of files.
Concepts like extents are used to reduce fragmentation and increase throughput.
It can use files as backing stores just as block devices to cut some kernel's
bookkeeping from write and read paths. It's far from perfect but it's work in
progress :)

There are three layers of caching:

1. OS page cache - holds compressed data,
2. page-level cache - operates on storage pages and holds compressed data,
3. node-level cache - holds decompressed nodes.

Replacement algorithm used is wTinyLFU which utilizes statistical tracking of
many more items than other algorithms are capable of which allows for much
better predictions on what data is going to be needed in the future. 

Databases in general are IO and network bound. This translates to making many
system calls which translates to making many contexts switches. Context switches
are expensive, especially since side-channel attack mitigations were put in
place. Luckly, a new interface in Linux kernel called io_uring was added which
allows for system call batching and asynchronous IO. This created an opportunity
to go a step further and eliminate multithreading thus reducing the need for
context switches even more. tyrdbs makes heavy use of a concept called
cooperative multithreading which means that every time a thread has to do some
sort of IO, it tells the system what to do and relinquishes control of the
processor to another thread. For database workloads, this is a perfect fit.
For now, tyrdbs works only on x86_64 systems.

Status
====

The database shows extraordinary performance for many use-cases but in the same
time, it is in a very early stage of development. There's no documentation apart
from this document. Many components are missing and some need further
optimizations. Things may or may not work as intended.

Contribution
====

If you'd like to contribute, we'll gladly accept PRs, ideas, critiques or
whatever else you can spare :) As mentioned in the [Status](#Status)
section, there's much to do. Most notably, networking protocol is basically
non-existent at the moment. As is transaction log. The list goes on.

License
====

All material in this project, except for what can be found under tyrtech/extern
directory, is released under
[Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0.html)
Copyright (C) Hrvoje Zeba. Text of this license can also be found in LICENSE
file. Material in tyrtech/extern is licensed according to their respective
licenses. Consult tyrtech/extern/SOURCES for more information.

Motivation and database manifesto - Why another database?
====

The hypothesis -- backed by personal experience and anecdotal evidence -- is
that companies, when they reach a certain scale, shift their focus from solving
problems in their respective business spaces to solving problems in databases
space. Common pattern is that when a company is first starting out, they pick
whatever is available at that particular moment either because of simplicity,
technology familiarity, pricing limitations or any combination of these three in
an effort to get the ball rolling quickly. After that, everything else that gets
developed is in some way or another tied to this specific database technology.
This includes but it's not limited to developing around that technology's
idiosynchronicities. If -- for any reason -- this technology isn't able to keep
up with the growth, company is stuck with these four options:

1. Scale down,
2. Change the technology,
3. Develop their own,
4. Work around technology's shortcomings.

For most companies scaling down is not an option so they are left with the other
three options.

If they decide to change the technology, drop-in and scalable replacements don't
really exist. They might exist in theory but practice is another matter. Finding
something that has similar idiosynchronicities and works on scale is hard. This
usually leads to companies having to adapt -- if not redesign -- parts of the code
that's already in production. This takes time. People within the company also
need time to learn this new technology to be able to utilize it effectively.
Combining all this with the fact that data migration is required -- which by
itself can be a big task -- it's not hard to see why most don't take this option.

Developing their own database technology is a lengthy process and often
requires very different skill sets. Most companies don't have the resources to
pull this off.

Since changing technologies or developing their own is viewed as hard, and for
the fact that scale requirements usually come gradually, most companies --
consciously or not -- choose the fourth option. Common way this is done is that
companies create a layer -- or layers -- on top of the database technology to be
able hide shortcomings and to implement various business requirements. On its
own this is a sound way of solving these kinds of problems but combined with the
scale requirements often fail in interesting ways. If, for example, the
underlying technology doesn't support horizontal scaling, this feature needs to
be implemented in the layer above it. This lands them right in the database and
distributed computing space. In essence, they gradually develop their own highly
specific database technology. Most miss that fact and don't put enough resources
into building a robust solution.

With all this in mind, developing a system which enables users to create their
own databases suited for their specific needs in an easier and more robust way
seems like a good idea.

Thank you
====

Special thanks needs to go to Jens Axboe and the team on io-uring mailing list
for making all of this possible!
