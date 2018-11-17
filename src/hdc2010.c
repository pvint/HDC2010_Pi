// Get temperature(s) from HDC2010 device(s)
//


#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <argp.h>

#define hdc2010_VERSION_MAJOR 0
#define hdc2010_VERSION_MINOR 0

#define HDC2010_TEMP_LSB 0x00
#define HDC2010_TEMP_MSB 0x01
#define HDC2010_HUMIDITY_LSB 0x02
#define HDC2010_HUMIDITY_MSB 0x03
#define HDC2010_CONFIG 0x0f

// CLI Arguments
// argp
const char *argp_program_version = "hdc2010_pi";
const char *argp_program_bug_address = "pjvint@gmail.com";

static char doc[] = "hdc2010 - simple CLI application to read thermocouple temperature with an HDC2010 device";
static char args_doc[] = "ARG1 [STRING...]";


static struct argp_option options[] = {
	{ "bus", 'b', "BUS", 0, "Bus number" },
	{ "address", 'a', "ADDRESS", 0, "Address (ie 0x40)" },
	{ "ambient", 'A', 0, 0, "Read cold junction temperature" },
	{ "time", 'e', 0, 0, "display elapsed time before temperature" },
	{ "resolution", 'r', "RESOLUTION", 0, "ADC Resolution. 0-3, 0=Max (18bit) 3 = min (12 bit)" },
	{ "thermocouple", 't', "THERMOCOUPLE", 0, "Thermocouple type" },
	{ "filter", 'f', "FILTER", 0, "Filter coeffocient" },
	{ "delay", 'd', "DELAY", 0, "Loop delay (ms) (if not set display once and exit)" },
	{ "quiet", 'q', 0, 0, "Suppress normal output, return temperature" },
	{ "verbose", 'v', "VERBOSITY", 0, "Verbose output" },
	{ "help", 'h', 0, 0, "Show help" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];                /* arg1 & arg2 */
	unsigned int bus, address, verbose, resolution, filter, ambient, delay, quiet, elapsedTime;
	char *thermocouple;
};

/* Parse a single option. */
static error_t parse_opt ( int key, char *arg, struct argp_state *state )
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch ( key )
	{
		case 'b':
			arguments->bus = atoi( arg );
			break;
		case 'a':
			arguments->address = strtoul( arg, NULL, 16 );
			break;
		case 'e':
			arguments->elapsedTime = 1;
			break;
		case 'v':
			arguments->verbose = strtoul( arg, NULL, 10 );
			break;
		case 'r':
			arguments->resolution = atoi( arg );
			break;
		case 't':
			arguments->thermocouple = arg;
			break;
		case 'd':
			arguments->delay = atoi( arg ) * 1000;
			break;
		case 'f':
			arguments->filter = atoi( arg );
			break;
		case 'A':
			arguments->ambient = 1;
			break;
		case 'q':
			arguments->quiet = 1;
			break;
		case 'h':
			//print_usage( "hdc2010" );
			printf("Try --usage\n");
			exit( 0 );
			break;
		case ARGP_KEY_ARG:
			if ( state->arg_num >= 2 )
			{
				argp_usage( state );
			}
			arguments->args[ state->arg_num ] = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

// TODO: Add syslog ability
void printLog( char *msg, unsigned int verbose, unsigned int level )
{
	if ( level > verbose )
		return;

	fprintf( stderr, "%d: %s\n", level, msg );
	return;
}


float readTemp( int file, unsigned int address )
{
	ioctl( file, I2C_SLAVE, address );

	char cfg[2];
	char reg1[1];

	// Test getting ID
	//char r[1] = {0x20};
	//write( file, r, 1 );
	//read( file, cfg, 2 );
	//printf("\nDeviceID: %02x %02x\n", cfg[0], cfg[1] );

	//cfg[0] = 0x05;
	//cfg[1] = 0x00;

	//write( file, cfg, 2 );

	//cfg[0] = 0x06;
	//cfg[1] = 0x00;

	//char reg1[1] = {0x04};
	//write(file, reg1, 1);

	// Wait for status bit to be high
	/*reg1[0] = 0x04;
	write( file, reg1, 1 );

	while ( (read( file, reg1, 1 ) && ( 1 << 6 )) == 0 )
	{
		usleep( 100000 );	// wait 100 ms
	}

	// clear the status bit (also sets "burst complete" bit to zero, but not important)
	write( file, reg1, 0 );
	*/

	// Get TEMP LSB
	reg1[0] = HDC2010_TEMP_LSB;
	write(file, reg1, 1);
	char data[2] = {0};
	read( file, data, 2 );
	int tempLSB = data[0];

	// Get TEMP MSB
	reg1[0] = HDC2010_TEMP_LSB;
        write(file, reg1, 1);
        read( file, data, 2 );
        int tempMSB = data[0];

	float ret = ( (float) (( tempMSB << 8 ) | tempLSB ) / ( 1 << 16 ) * 165 - 40 );

	//printf("%f\n", ret);
	return ret;
}

float readAmbientTemp( int file, unsigned int address )
{
	ioctl( file, I2C_SLAVE, address );

	char cfg[2];

	char reg1[1] = {0x02};
	write(file, reg1, 1);
	char data[2] = {0};
	read( file, data, 2 );

	int lowTemp = data[0] & 0x80;
	float ret;
	if ( lowTemp )
	{
		ret = data[0] * 16 + data[1] / 16 - 4096;
	}
	else
	{
		ret = data[0] * 16 + data[1] * 0.0625;
	}
	//printf("%f\n", ret);
	return ret;
}

int main( int argc, char **argv )
{
	struct arguments arguments;

	/* Default values. */
	arguments.bus = 1;
	arguments.address = 0x40;
	arguments.verbose = 0;
	arguments.resolution = 0;
	arguments.thermocouple = 'K';
	arguments.delay = 0;
	arguments.filter = 0;
	arguments.ambient = 0;
	arguments.elapsedTime = 0;
	arguments.quiet = 0;

	/* Parse our arguments; every option seen by parse_opt will
	be reflected in arguments. */
	argp_parse ( &argp, argc, argv, 0, 0, &arguments );

	unsigned char config = arguments.resolution;

	//int file = sensorConfig( arguments.bus, arguments.address, arguments.thermocouple, arguments.filter, config );
	int file;

        file = initHardware ( arguments.bus, arguments.address );

        ioctl( file, I2C_SLAVE, arguments.address );


	// start timer
	struct timeval tv1, tv2;
	gettimeofday( &tv1, NULL );

	while ( 1 )
	{
		float temp = readTemp( file, arguments.address );

		if ( arguments.quiet != 0 )
		{
			return (int) temp;
		}

		if ( arguments.elapsedTime )
		{
			gettimeofday( &tv2, NULL );
			printf("%.3f ", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec) );
		}


		printf("%.2f\n", temp);

		if ( arguments.delay == 0 )
		{
			break;
		}

		usleep ( arguments.delay );
	}

	return 0;
}

int initHardware( unsigned int adpt, unsigned int addr )
{
	int file;
	char bus[11];

	sprintf( bus, "/dev/i2c-%01d", adpt );

	if ( ( file = open( bus, O_RDWR ) ) < 0 )
	{
		char msg[256];
		snprintf( msg, 256, "Error opening %s\n", bus );
		printLog( msg, 1, 1 );
		exit( 1 );
	}

	//ioctl( file, I2C_SLAVE, addr );

	return file;
}
