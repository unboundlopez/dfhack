#include <integers.h>
#include <string.h> // for memset
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <bitset>
using namespace std;

#include <sstream>
#include "fake-curses.h"
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include <math.h>

#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>
#include <modules/Maps.h>
#include <modules/Materials.h>
#include "miscutils.h"
using namespace DFHack;


string error;
API * pDF = 0;


struct t_tempz
{
    int32_t limit;
    int character;
};

t_tempz temp_limits[]=
{
    {50, '.'},
    {100, '+'},
    {500, '*'},
    {1000, '#'},
    {2000, '!'}
};
#define NUM_LIMITS 5

static void finish(int sig);

int gotoxy(int x, int y)
{
    wmove(stdscr, y , x );
    return 0;
}

int putch(int x, int y, int znak, int color)
{
    attron(COLOR_PAIR(color));
    mvwaddch(stdscr, y, x, znak);
    attroff(COLOR_PAIR(color));
}
int putwch(int x, int y, int znak, int color)
{
    attron(COLOR_PAIR(color));
    mvwaddch(stdscr, y, x, znak);
    attroff(COLOR_PAIR(color));
}
/*
    enum TileClass
    {
        EMPTY,
        
        WALL,
        PILLAR,
        FORTIFICATION,
        
        STAIR_UP,
        STAIR_DOWN,
        STAIR_UPDOWN,
        
        RAMP,
        
        FLOOR,
        TREE_DEAD,
        TREE_OK,
        SAPLING_DEAD,
        SAPLING_OK,
        SHRUB_DEAD,
        SHRUB_OK,
        BOULDER,
        PEBBLES
    };*/

int puttile(int x, int y, int tiletype, int color)
{
    unsigned int znak;
    switch(tileTypeTable[tiletype].c)
    {
        case EMPTY:
            znak = ' ';
            break;
        case PILLAR:
        case WALL:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u2593");
            attroff(COLOR_PAIR(color));
            //znak = ;
            return 0;
        case FORTIFICATION:
            znak = '#';
            break;
        case STAIR_DOWN:
            znak = '>';
            break;
        case STAIR_UP:
            znak = '<';
            break;
        case STAIR_UPDOWN:
            znak = '=';
            break;
        case RAMP:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u25B2");
            attroff(COLOR_PAIR(color));
            return 0;
        case RAMP_TOP:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u25BC");
            attroff(COLOR_PAIR(color));
            return 0;
        case FLOOR:
            znak = '.';
            break;
        case TREE_DEAD:
        case TREE_OK:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u2663");
            attroff(COLOR_PAIR(color));
            return 0;
        case SAPLING_DEAD:
        case SAPLING_OK:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u03C4");
            attroff(COLOR_PAIR(color));
            return 0;
        case SHRUB_DEAD:
        case SHRUB_OK:
            attron(COLOR_PAIR(color));
            mvwaddwstr(stdscr, y, x, L"\u2666");
            attroff(COLOR_PAIR(color));
            return 0;
        case BOULDER:
        case PEBBLES:
            znak= '*';
            break;
    }
    attron(COLOR_PAIR(color));
    mvwaddch(stdscr, y, x, znak);
    attroff(COLOR_PAIR(color));
}

int cprintf(char *fmt, ...)
{
    va_list ap; 
    va_start(ap, fmt);
    int i = vwprintw(stdscr,fmt, ap);
    va_end(ap);
    return i;
}

void clrscr()
{
    wbkgd(stdscr, COLOR_PAIR(COLOR_BLACK));
    wclear(stdscr);
}

/*
    enum TileMaterial
    {
        AIR,
        SOIL,
        STONE,
        FEATSTONE, // whatever it is
        OBSIDIAN,
        
        VEIN,
        ICE,
        GRASS,
        GRASS2,
        GRASS_DEAD,
        GRASS_DRY,
        DRIFTWOOD,
        HFS,
        MAGMA,
        CAMPFIRE,
        FIRE,
        ASHES,
        CONSTRUCTED
    };
*/
int pickColor(int tiletype)
{
    switch(tileTypeTable[tiletype].m)
    {
        case AIR:
            return COLOR_BLACK;
        case STONE:
        case FEATSTONE:
        case OBSIDIAN:
        case CONSTRUCTED:
        case ASHES:
        default:
            return COLOR_WHITE;
        case SOIL:
        case GRASS_DEAD:
        case GRASS_DRY:
        case DRIFTWOOD:
            return COLOR_YELLOW;
        case ICE:
            return COLOR_CYAN;
        case VEIN:
            return COLOR_MAGENTA;
        case GRASS:
        case GRASS2:
            return COLOR_GREEN;
        case HFS:
        case MAGMA:
        case CAMPFIRE:
        case FIRE:
            return COLOR_RED;
    }

}

/*
address = absolute address of dump start
length = length in bytes
*/
void hexdump (DFHack::API& DF, uint32_t address, uint32_t length, int filenum)
{
    uint32_t reallength;
    uint32_t lines;
    lines = (length / 16) + 1;
    reallength = lines * 16;
    char *buf = new char[reallength];
    ofstream myfile;
    
    stringstream ss;
    ss << "hexdump" << filenum << ".txt";
    string name = ss.str();
    
    myfile.open (name.c_str());
    
    DF.ReadRaw(address, reallength, (uint8_t *) buf);
    for (int i = 0; i < lines; i++)
    {
        // leading offset
        myfile << "0x" << hex << setw(4) << i*16 << " ";
        // groups
        for(int j = 0; j < 4; j++)
        {
            // bytes
            for(int k = 0; k < 4; k++)
            {
                int idx = i * 16 + j * 4 + k;
                
                myfile << hex << setw(2) << int(static_cast<unsigned char>(buf[idx])) << " ";
            }
            myfile << " ";
        }
        myfile << endl;
    }
    delete buf;
    myfile.close();
}


main(int argc, char *argv[])
{
    /* initialize your non-curses data structures here */

    signal(SIGINT, finish);      /* arrange interrupts to terminate */
    setlocale(LC_ALL,"");
    initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    noecho();       /* don't echo input */
    //nodelay(stdscr, true); 
    int wxMax = getmaxx(stdscr);
    int wyMax = getmaxy(stdscr);

    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    if (has_colors())
    {
        start_color();

        /*
         * Simple color assignment, often all we need.
         */
        init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);

        init_color(COLOR_CYAN, 700, 700, 700); // lt grey
        init_color(COLOR_MAGENTA, 500, 500, 500); // dk grey
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    }
    
    int x_max,y_max,z_max;
    uint32_t x_max_a,y_max_a,z_max_a;
    /*
    uint16_t tiletypes[16][16];
    DFHack::t_designation designations[16][16];
    uint8_t regionoffsets[16];
    */
    map <int16_t, uint32_t> materials;
    materials.clear();
    mapblock40d blocks[3][3];
    vector<DFHack::t_matgloss> stonetypes;
    vector<DFHack::t_matgloss> creature_types;
    vector<DFHack::t_effect_df40d> effects;
    vector< vector <uint16_t> > layerassign;
    vector<t_vein> veinVector;
    vector<t_frozenliquidvein> IceVeinVector;
    vector<t_spattervein> splatter;
    t_temperatures b_temp1;
    t_temperatures b_temp2;

    DFHack::Materials * Mats = 0;
    DFHack::Maps * Maps = 0;
    
    
    DFHack::API DF("Memory.xml");
    try
    {
        DF.Attach();
        Mats = DF.getMaterials();
        Maps = DF.getMaps();
        pDF = &DF;
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        finish(0);
    }
    
    Process* p = DF.getProcess();
    // init the map
    if(!Maps->Start())
    {
        error = "Can't find a map to look at.";
        pDF = 0;
        finish(0);
    }
    
    Maps->getSize(x_max_a,y_max_a,z_max_a);
    x_max = x_max_a;
    y_max = y_max_a;
    z_max = z_max_a;
    
    // get stone matgloss mapping
    if(!Mats->ReadInorganicMaterials(stonetypes))
    {
        error = "Can't read stone types.";
        pDF = 0;
        finish(0);
    }
    
    if(!Mats->ReadCreatureTypes(creature_types))
    {
        error = "Can't read stone types.";
        pDF = 0;
        finish(0);
    }
/*
    // get region geology
    if(!DF.ReadGeology( layerassign ))
    {
        error = "Can't read local geology.";
        pDF = 0;
        finish(0);
    }
*/
    // FIXME: could fail on small forts
    int cursorX = x_max/2 - 1;
    int cursorY = y_max/2 - 1;
    int cursorZ = z_max/2 - 1;
    
    
    bool dig = false;
    bool dump = false;
    bool digbit = false;
    bool dotwiddle;
    unsigned char twiddle = 0;
    int vein = 0;
    int filenum = 0;
    bool dirtybit = false;
    uint32_t blockaddr = 0;
    uint32_t blockaddr2 = 0;
    t_blockflags bflags;
    bflags.whole = 0;
    enum e_tempmode
    {
        TEMP_NO,
        TEMP_1,
        TEMP_2
    };
    e_tempmode temperature = TEMP_NO;
    
    // resume so we don't block DF while we wait for input
    DF.Resume();
    
    for (;;)
    {
        dig = false;
        dump = false;
        dotwiddle = false;
        digbit = false;
        
        int c = getch();     /* refresh, accept single keystroke of input */
        flushinp();
        clrscr();
        /* process the command keystroke */
        switch(c)
        {
            case KEY_DOWN:
                cursorY ++;
                break;
            case KEY_UP:
                cursorY --;
                break;
            case KEY_LEFT:
                cursorX --;
                break;
            case KEY_RIGHT:
                cursorX ++;
                break;
            case KEY_NPAGE:
                cursorZ --;
                break;
            case KEY_PPAGE:
                cursorZ ++;
                break;
            case '+':
                vein ++;
                break;
            case 'd':
                dig = true;
                break;
            case 'o':
                dump = true;
                break;
            case '-':
                vein --;
                break;
            case 'z':
                digbit = true;
                break;
            case '/':
                if(twiddle != 0) twiddle--;
                break;
            case '*':
                twiddle++;
                break;
            case 't':
                dotwiddle = true;
                break;
            case 'b':
                temperature = TEMP_NO;
                break;
            case 'n':
                temperature = TEMP_1;
                break;
            case 'm':
                temperature = TEMP_2;
                break;
            default:
                break;
        }
        cursorX = max(cursorX, 0);
        cursorY = max(cursorY, 0);
        cursorZ = max(cursorZ, 0);
        
        cursorX = min(cursorX, x_max - 1);
        cursorY = min(cursorY, y_max - 1);
        cursorZ = min(cursorZ, z_max - 1);
        
        if(twiddle > 31)
            twiddle = 31;
        
        // clear data before we suspend
        memset(blocks,0,sizeof(blocks));
        veinVector.clear();
        IceVeinVector.clear();
        effects.clear();
        splatter.clear();
        dirtybit = 0;
        
        // Supend, read/write data
        DF.Suspend();
        uint32_t effectnum;
        /*
        if(DF.InitReadEffects(effectnum))
        {
            for(uint32_t i = 0; i < effectnum;i++)
            {
                t_effect_df40d effect;
                DF.ReadEffect(i,effect);
                effects.push_back(effect);
            }
        }
        */
        for(int i = -1; i <= 1; i++) for(int j = -1; j <= 1; j++)
        {
            mapblock40d * Block = &blocks[i+1][j+1];
            
            
            if(Maps->isValidBlock(cursorX+i,cursorY+j,cursorZ))
            {
                Maps->ReadBlock40d(cursorX+i,cursorY+j,cursorZ, Block);
                
                // extra processing of the block in the middle
                if(i == 0 && j == 0)
                {
                    // read veins
                    Maps->ReadVeins(cursorX+i,cursorY+j,cursorZ,&veinVector,&IceVeinVector,&splatter);
                    
                    // get pointer to block
                    blockaddr = Maps->getBlockPtr(cursorX+i,cursorY+j,cursorZ);
                    blockaddr2 = Block->origin;
                    
                    // dig all veins and trees
                    if(dig)
                    {
                        for(int x = 0; x < 16; x++) for(int y = 0; y < 16; y++)
                        {
                            TileClass tc = tileTypeTable[Block->tiletypes[x][y]].c;
                            TileMaterial tm = tileTypeTable[Block->tiletypes[x][y]].m;
                            if( tc == WALL && tm == VEIN || tc == TREE_OK || tc == TREE_DEAD)
                            {
                                Block->designation[x][y].bits.dig = designation_default;
                            }
                        }
                        Maps->WriteDesignations(cursorX+i,cursorY+j,cursorZ, &(Block->designation));
                    }
                    
                    // read temperature data
                    Maps->ReadTemperatures(cursorX+i,cursorY+j,cursorZ,&b_temp1, &b_temp2 );
                    if(dotwiddle)
                    {
                        bitset<32> bs = Block->designation[0][0].whole;
                        bs.flip(twiddle);
                        Block->designation[0][0].whole = bs.to_ulong();
                        Maps->WriteDesignations(cursorX+i,cursorY+j,cursorZ, &(Block->designation));
                        dotwiddle = false;
                    }
                    
                    // do a dump of the block data
                    if(dump)
                    {
                        hexdump(DF,blockaddr,0x1E00,filenum);
                        filenum++;
                    }
                    // read/write dirty bit of the block
                    Maps->ReadDirtyBit(cursorX+i,cursorY+j,cursorZ,dirtybit);
                    Maps->ReadBlockFlags(cursorX+i,cursorY+j,cursorZ,bflags);
                    if(digbit)
                    {
                        dirtybit = !dirtybit;
                        Maps->WriteDirtyBit(cursorX+i,cursorY+j,cursorZ,dirtybit);
                    }
                }
            }
        }
        // Resume, print stuff to the terminal
        DF.Resume();
        for(int i = -1; i <= 1; i++) for(int j = -1; j <= 1; j++)
        {
            mapblock40d * Block = &blocks[i+1][j+1];
            for(int x = 0; x < 16; x++) for(int y = 0; y < 16; y++)
            {
                int color = COLOR_BLACK;
                color = pickColor(Block->tiletypes[x][y]);
                /*
                if(!Block->designation[x][y].bits.hidden)
                {
                    puttile(x+(i+1)*16,y+(j+1)*16,Block->tiletypes[x][y], color);
                }
                else*/
                {
                    
                    attron(A_STANDOUT);
                    puttile(x+(i+1)*16,y+(j+1)*16,Block->tiletypes[x][y], color);
                    attroff(A_STANDOUT);
                    
                }
            }
            // print effects for the center tile
            /*
            if(i == 0 && j == 0)
            {
                for(uint zz = 0; zz < effects.size();zz++)
                {
                    if(effects[zz].z == cursorZ && !effects[zz].isHidden)
                    {
                        // block coords to tile coords
                        uint16_t x = effects[zz].x - (cursorX * 16);
                        uint16_t y = effects[zz].y - (cursorY * 16);
                        if(x < 16 && y < 16)
                        {
                            putch(x + 16,y + 16,'@',COLOR_WHITE);
                        }
                    }
                }
            }
            */
        }
        gotoxy(50,0);
        cprintf("arrow keys, PGUP, PGDN = navigate");
        gotoxy(50,1);
        cprintf("+,-                    = switch vein");
        gotoxy(50,2);
        uint32_t mineralsize = veinVector.size();
        uint32_t icesize = IceVeinVector.size();
        uint32_t splattersize = splatter.size();
        uint32_t totalVeinSize =  mineralsize+ icesize + splattersize;
        if(vein == totalVeinSize) vein = totalVeinSize - 1;
        if(vein < -1) vein = -1;
        cprintf("X %d/%d, Y %d/%d, Z %d/%d. Vein %d of %d",cursorX+1,x_max,cursorY+1,y_max,cursorZ,z_max,vein+1,totalVeinSize);
        if(!veinVector.empty() || !IceVeinVector.empty() || !splatter.empty())
        {
            if(vein != -1 && vein < totalVeinSize)
            {
                uint32_t realvein = 0;
                if(vein < mineralsize)
                {
                    realvein = vein;
                    //iterate through vein rows
                    for(uint32_t j = 0;j<16;j++)
                    {
                        //iterate through the bits
                        for (uint32_t k = 0; k< 16;k++)
                        {
                            /*
                            if(tileTypeTable[blocks[1][1].tiletypes[k][j]].m != VEIN)
                                continue;
                            */
                            /*
                            if(blocks[1][1].designation[k][j].bits.hidden)
                                continue;
                            */
                            // and the bit array with a one-bit mask, check if the bit is set
                            bool set = !!(((1 << k) & veinVector[realvein].assignment[j]) >> k);
                            if(set)
                            {
                                putch(k+16,j+16,'$',COLOR_RED);
                            }
                        }
                    }
                    gotoxy(50,3);
                    cprintf("Mineral: %s",stonetypes[veinVector[vein].type].id);
                }
                else if (vein < mineralsize + icesize)
                {
                    realvein = vein - mineralsize;
                    t_frozenliquidvein &frozen = IceVeinVector[realvein];
                    for(uint32_t i = 0;i<16;i++)
                    {
                        for (uint32_t j = 0; j< 16;j++)
                        {
                            int color = COLOR_BLACK;
                            int tile = frozen.tiles[i][j];
                            color = pickColor(tile);
                            
                            attron(A_STANDOUT);
                            puttile(i+16,j+16,tile, color);
                            attroff(A_STANDOUT);
                        }
                    }
                    gotoxy(50,3);
                    cprintf("ICE");
                }
                else
                {
                    realvein = vein - mineralsize - icesize;
                    t_spattervein &bloodmud = splatter[realvein];
                    for(uint32_t yyy = 0; yyy < 16; yyy++)
                    {
                        for(uint32_t xxx = 0; xxx < 16; xxx++) 
                        {
                            uint8_t intensity = splatter[realvein].intensity[xxx][yyy];
                            if(intensity)
                            {
                                attron(A_STANDOUT);
                                putch(xxx+16,yyy+16,'*', COLOR_RED);
                                attroff(A_STANDOUT);
                            }
                        }
                    }
                    gotoxy(50,3);
                    cprintf("Spatter: %s",PrintSplatterType(splatter[realvein].mat1,splatter[realvein].mat2,creature_types).c_str());
                }
            }
        }
        mapblock40d * Block = &blocks[1][1];
        t_temperatures * ourtemp;
        if(temperature == TEMP_NO)
        {
            for(int x = 0; x < 16; x++) for(int y = 0; y < 16; y++)
            {
                if((Block->occupancy[x][y].whole & (1 << twiddle)))
                {
                    putch(x + 16,y + 16,'@',COLOR_WHITE);
                }
            }
        }
        else
        {
            if(temperature == TEMP_1)
                ourtemp = &b_temp1;
            else if(temperature == TEMP_2)
                ourtemp = &b_temp2;
            uint64_t sum = 0;
            uint16_t min, max;
            min = max = (*ourtemp)[0][0];
            for(int x = 0; x < 16; x++) for(int y = 0; y < 16; y++)
            {
                uint16_t temp = (*ourtemp)[x][y];
                if(temp < min) min = temp;
                if(temp > max) max = temp;
                sum += temp;
            }
            uint64_t average = sum/256;
            gotoxy (50,8);
            if(temperature == TEMP_1)
                cprintf ("temperature1 [°U] (min,avg,max): %d,%d,%d", min, average, max);
            else if(temperature == TEMP_2)
                cprintf ("temperature2 [°U] (min,avg,max): %d,%d,%d", min, average, max);
            
            for(int x = 0; x < 16; x++) for(int y = 0; y < 16; y++)
            {
                int32_t temper = (int32_t) (*ourtemp)[x][y];
                temper -= average;
                uint32_t abs_temp = abs(temper);
                int color;
                unsigned char character = ' ';
                if(temper >= 0)
                    color = COLOR_RED;
                else
                    color = COLOR_BLUE;
                
                for(int i = 0; i < NUM_LIMITS; i++)
                {
                    if(temp_limits[i].limit < abs_temp)
                        character = temp_limits[i].character;
                    else break;
                }
                if( character != ' ')
                {
                    putch(x + 16,y + 16,character,color);
                }
            }
        }
        gotoxy (50,4);
        cprintf("block address 0x%x, flags 0x%08x",blockaddr, bflags.whole);
        gotoxy (50,5);
        cprintf("dirty bit: %d, twiddle: %d",dirtybit,twiddle);
        gotoxy (50,6);
        cprintf ("d - dig veins, o - dump map block, z - toggle dirty bit");
        gotoxy (50,7);
        cprintf ("b - no temperature, n - temperature 1, m - temperature 2");
        wrefresh(stdscr);
    }
    pDF = 0;
    finish(0);
}

static void finish(int sig)
{
    // ugly
    if(pDF)
    {
        pDF->ForceResume();
        pDF->Detach();
    }
    endwin();
    if(!error.empty())
    {
        cerr << error << endl;
    }
    exit(0);
}