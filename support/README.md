This directory contains the Coral 66 library code and the external C code that comprises the support library.

Helper scripts are provided in the top level scripts directory to assist in building the support library, see
the [compile-supported](../scripts/compile-supported.sh) script.

Individual library and external build scripts are also provided, [build-externals](../scripts/build-externals.sh) and
[build-libraries](../scripts/build-libraries.sh). These allow the user to extend the support library as needed or indeed 
completely replace it with their own.

The support library is deliberately sparse, its purpose is to allow users to get up and running with Coral 66, not
to provide a comprehensive standard library implementation.

It is intended that additional library functionality will be added as required.