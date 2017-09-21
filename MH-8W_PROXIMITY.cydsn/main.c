/* =====================================================================================================================
* Copyright Bowers&Wilkins, 2017-07
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* PSOC_BLE Program for MH-8W project.
 -- BW_MH-8W_Release --
* =====================================================================================================================
*/ 
 
/************************************************************************************************************************
* Include
************************************************************************************************************************/
#include <project.h>  
   
/********************************************************************************************************
* Function Definitions
********************************************************************************************************/
//#define RUN_TUNER                   1
#define ROW_NUM                                 255
#define PROXIMITY_SETTING_FLASH_WRITE_PAGE      254
#define ROW_SIZE                                64

#define EEPROM_SIGNATURE_0                      0x55
#define EEPROM_SIGNATURE_1                      0xAA
 
#define BASELINE                                0u
#define DEFAULT_SENSITIVITY                     200u
#define PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH  600u
 
#define WDT_PERIOD                              (50 * 40)  

static const uint8 CYCODE eepStrg[ROW_SIZE]  __attribute__ ((section (".EEPROMDATA"))) = {0,0,0};

uint16  version                                             = 16u;
uint8   bRAMarray[ROW_SIZE];
uint16  wDefaultBaseline                                    = 0u;
uint16  g_proximity_min_raw_count                           = 0xFFFF;
uint16  g_proximity_min_raw_count_candidate_1               = 0xFFFF;
uint16  g_proximity_min_raw_count_candidate_2               = 0xFFFF;
uint16  g_proximity_min_off_auto_tracking_raw_count         = 0xFFFF;
uint16  g_proximity_sensitivity                             = DEFAULT_SENSITIVITY;        //PROX_ON_OFF_Delta (Sensitivity)
uint16  g_proximity_currect_raw_count                       = 0x0000;
uint16  g_proximity_currect_baseline                        = 0x0000;
uint16  g_proximity_diff                                    = 0u;    

uint16  g_proximity_max_raw_count                           = 0x0000;
uint16  g_proximity_max_diff                                = 0x0000;
uint16  g_proximity_on_when_power_on_raw_count              = 0x0000;
uint16  g_proximity_on_more_th_when_power_on_raw_count      = 0x0000;
uint16  g_proximity_when_power_on_check_candidate_raw_count = 000000;
uint16  g_proximity_on_when_power_on_baseline               = 0x0000;

uint8   g_proximity_threshold_first_time_check_on_flag      = 0u;
uint16  g_proximity_when_power_on_diff_on_count             = 0u;
uint8   g_proximity_always_on_flag                          = 0u;

uint8   bActiveCount    = 255;
uint8   MODE            = 0u;
uint8   bStatus; 

#define WRITE                           0x61     //write_proximity_TH
#define WRITE_PROXIMITY_MIN_RAW_COUNT   0x51     //WRITE_PROXIMITY_MIN_RAW_COUNT
#define WRITE_PROXIMITY_SENSITIVITY     0x52     //WRITE_PROXIMITY_SENSITIVITY
#define WRITE_MODE                      0x53     //WRITE_MODE
#define CYPRESS_ENTER_DFU               0x70
 
uint8   bI2C_Reg[16] ={0x00};
#define I2C_COMMAND                     0
#define I2C_DATA_MSB                    1
#define I2C_DATA_LSB                    2
 
#define I2C_CURRENT_RAW_COUNT_MSB       3
#define I2C_CURRENT_RAW_COUNT_LSB       4
#define I2C_THRESHOLD_COUNT_MSB         5
#define I2C_THRESHOLD_COUNT_LSB         6
#define I2C_PROXIMITY_MIN_MSB           7
#define I2C_PROXIMITY_MIN_LSB           8
#define I2C_PROXIMITY_SENSITIVITY_MSB   9
#define I2C_PROXIMITY_SENSITIVITY_LSB   10
#define I2C_VERSION                     11
#define I2C_PROXIMITY_BASELINE_MSB      12
#define I2C_PROXIMITY_BASELINE_LSB      13
#define I2C_PROXIMITY_DIFF_MSB          14
#define I2C_PROXIMITY_DIFF_LSB          15

#define MODE_DIFF_DATA                  1

unsigned char bPowerOnCheckTh = 1;


/*******************************************************************************
* Function Name: CY_ISR
*******************************************************************************/
CY_ISR(WDT_ISR)
{
    /* Clear WDT interrupt */
    CySysWdtClearInterrupt();
	   
    /* Write WDT_MATCH with current match value + desired match value */
    CySysWdtWriteMatch((uint16)CySysWdtReadMatch()+WDT_PERIOD);
    
    bActiveCount++;
}

/*******************************************************************************
* Function Name: EEPROM_Write()
*******************************************************************************/
void EEPROM_Write(void)
{
    CySysFlashWriteRow(PROXIMITY_SETTING_FLASH_WRITE_PAGE, bRAMarray);
    CyDelay(100);
}

/*******************************************************************************
* Function Name: EEPROM_Start()
*******************************************************************************/
void EEPROM_Start(void)
{
    uint8 bCount;
    
    for(bCount=0;bCount<ROW_SIZE;bCount++){
        bRAMarray[bCount] = eepStrg[bCount];
    }
    
    wDefaultBaseline = bRAMarray[2];
    wDefaultBaseline = wDefaultBaseline << 8;
    wDefaultBaseline = wDefaultBaseline & 0xFF00;
    wDefaultBaseline += bRAMarray[3];
    
    if(bRAMarray[4] != 0u && bRAMarray[5] != 0u)
    {
        g_proximity_min_raw_count = (bRAMarray[4] << 8) + bRAMarray[5];
    }
    
    g_proximity_sensitivity = (bRAMarray[6] << 8) + bRAMarray[7];
    
    
    if( (bRAMarray[0] != EEPROM_SIGNATURE_0) || (bRAMarray[1] != EEPROM_SIGNATURE_1) )
    {
        bRAMarray[0] = EEPROM_SIGNATURE_0;
        bRAMarray[1] = EEPROM_SIGNATURE_1;
        
        wDefaultBaseline = BASELINE;
        bRAMarray[2] = wDefaultBaseline >> 8;
        bRAMarray[3] = wDefaultBaseline & 0xFF;
        
        g_proximity_min_raw_count = 0xFFFFu;
        bRAMarray[4] = g_proximity_min_raw_count >> 8;
        bRAMarray[5] = g_proximity_min_raw_count & 0xFF;
        
        g_proximity_sensitivity = DEFAULT_SENSITIVITY;
        bRAMarray[6] = g_proximity_sensitivity >> 8;
        bRAMarray[7] = g_proximity_sensitivity & 0xFF;
        
        //EEPROM_Write();
    }
    
    bI2C_Reg[I2C_THRESHOLD_COUNT_MSB]       = wDefaultBaseline >> 8;
    bI2C_Reg[I2C_THRESHOLD_COUNT_LSB]       = wDefaultBaseline & 0xFF;
    bI2C_Reg[I2C_PROXIMITY_MIN_MSB]         = g_proximity_min_raw_count >> 8;
    bI2C_Reg[I2C_PROXIMITY_MIN_LSB]         = g_proximity_min_raw_count & 0xFF;
    bI2C_Reg[I2C_PROXIMITY_SENSITIVITY_MSB] = g_proximity_sensitivity >> 8;
    bI2C_Reg[I2C_PROXIMITY_SENSITIVITY_LSB] = g_proximity_sensitivity & 0xFF;
    bI2C_Reg[I2C_VERSION]                   = version;
}

/*******************************************************************************
* Function Name: WDT_Start()
*******************************************************************************/
void WDT_Start()
{
    /* Enable global interrupts before starting CapSense and EzI2C blocks*/
    /* Write WDT_MATCH with the desired match value */
    CySysWdtWriteMatch(WDT_PERIOD);
     
     /* Mask WDT interrupt to be available for CPU service */
    CySysWdtUnmaskInterrupt();
     
     /* Set the WDT interrupt vector to the WDT ISR*/
    CyIntSetVector(4, WDT_ISR);
     
    /* Enable WDT ISR in CPU vectors - which is caused by WDT match */
    CyIntEnable(4);   
}


/*******************************************************************************
* Function Name: CapSense_Tuner()
*******************************************************************************/
void CapSense_Tuner(void)
{
    EZI2C_Start();	 		/* Initialize EZI2C block */
    /* Make CapSense RAM data structure an I2C buffer, to use CapSense Tuner */
    EZI2C_EzI2CSetBuffer1(sizeof(CapSense_dsRam), sizeof(CapSense_dsRam), (uint8 *)&CapSense_dsRam);
 
    CapSense_Start();		     /* Initialize CapSense Component */	
    CapSense_ScanAllWidgets();   /* Scan all widgets */
    
    for(;;)
    {
        if(CapSense_NOT_BUSY == CapSense_IsBusy())
        {   
            CapSense_ProcessAllWidgets();   /* Process data for all widgets */
            
            if (CapSense_IsWidgetActive(CapSense_PROXIMITY0_WDGT_ID))
            {
                Pin_interrupt_Write(1);
            }
            else
            {
                Pin_interrupt_Write(0);   
            }
                        
            /* Sync CapSense parameters with those set via CapSense tuner before 
             * the beginning of new CapSense scan */
            CapSense_RunTuner();

            /* Initiate new scan for all widgets */
            CapSense_ScanAllWidgets();                      
        }
    }
}


/************************************************************************************************************************
* Function Name: MAIN
************************************************************************************************************************/
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    EEPROM_Start();
    WDT_Start();
    
    #ifdef RUN_TUNER
        CapSense_Tuner();
    #endif
    
    CapSense_Start();		     // Initialize CapSense Component 
    
    EZI2C_Start();	 		
    EZI2C_EzI2CSetBuffer1(sizeof(bI2C_Reg), sizeof(bI2C_Reg) , (uint8 *)&bI2C_Reg);
    

    for(;;)
    {
        //I2C_SLAVE
        //-------------------------------------------------------------------------------------------------------------------------------------- 
        if( (EZI2C_EzI2CGetActivity() & EZI2C_EZI2C_STATUS_BUSY) == 0)
        {
            //WRITE_PROXIMITY_TH   
            if(bI2C_Reg[I2C_COMMAND] == WRITE)
            {   
                bRAMarray[0] = EEPROM_SIGNATURE_0;
                bRAMarray[1] = EEPROM_SIGNATURE_1;
                bRAMarray[2] = bI2C_Reg[I2C_DATA_MSB];
                bRAMarray[3] = bI2C_Reg[I2C_DATA_LSB];
                bRAMarray[4] = g_proximity_min_raw_count >> 8;
                bRAMarray[5] = g_proximity_min_raw_count & 0xFF;
                bRAMarray[6] = g_proximity_sensitivity >> 8;
                bRAMarray[7] = g_proximity_sensitivity & 0xFF;
                
                wDefaultBaseline = bRAMarray[2];
                wDefaultBaseline = wDefaultBaseline << 8;
                wDefaultBaseline = wDefaultBaseline & 0xFF00;
                wDefaultBaseline += bRAMarray[3];
            
                EEPROM_Write();
                
                bI2C_Reg[I2C_COMMAND]  = 0;
                bI2C_Reg[I2C_DATA_MSB] = 0;
                bI2C_Reg[I2C_DATA_LSB] = 0;
             
                bI2C_Reg[I2C_THRESHOLD_COUNT_MSB] =  wDefaultBaseline >> 8u;
                bI2C_Reg[I2C_THRESHOLD_COUNT_LSB] =  wDefaultBaseline & 0xFF;
            }
            
            //WRITE_PROXIMITY_MIN_RAW_COUNT
            if(bI2C_Reg[I2C_COMMAND] == WRITE_PROXIMITY_MIN_RAW_COUNT)
            {
                bRAMarray[0] = EEPROM_SIGNATURE_0;
                bRAMarray[1] = EEPROM_SIGNATURE_1;
                bRAMarray[2] = wDefaultBaseline >> 8;
                bRAMarray[3] = wDefaultBaseline & 0xFF;
                
                if(bI2C_Reg[I2C_DATA_MSB] == 0xFF && bI2C_Reg[I2C_DATA_LSB] == 0xFF)
                {
                    bRAMarray[4] = 0xFF;
                    bRAMarray[5] = 0xFF;
                }
                else
                {
                    bRAMarray[4] = g_proximity_min_raw_count >> 8u;
                    bRAMarray[5] = g_proximity_min_raw_count & 0xFF;
                }
                
                bRAMarray[6] = g_proximity_sensitivity >> 8;
                bRAMarray[7] = g_proximity_sensitivity & 0xFF;
                
                EEPROM_Write();
                
                bI2C_Reg[I2C_COMMAND]  = 0;
                bI2C_Reg[I2C_DATA_MSB] = 0;
                bI2C_Reg[I2C_DATA_LSB] = 0;
                
                bI2C_Reg[I2C_PROXIMITY_MIN_MSB] =  g_proximity_sensitivity >> 8u;
                bI2C_Reg[I2C_PROXIMITY_MIN_MSB] =  g_proximity_sensitivity & 0xFF;
            }
            
            //WRITE_PROXIMITY_SENSITIVITY
            if(bI2C_Reg[I2C_COMMAND] == WRITE_PROXIMITY_SENSITIVITY)
            {
                bRAMarray[0] = EEPROM_SIGNATURE_0;
                bRAMarray[1] = EEPROM_SIGNATURE_1;
                bRAMarray[2] = wDefaultBaseline >> 8;
                bRAMarray[3] = wDefaultBaseline & 0xFF;
                bRAMarray[4] = g_proximity_min_raw_count >> 8;
                bRAMarray[5] = g_proximity_min_raw_count & 0xFF;
                bRAMarray[6] = bI2C_Reg[I2C_DATA_MSB];
                bRAMarray[7] = bI2C_Reg[I2C_DATA_LSB];
                
                g_proximity_sensitivity = (bRAMarray[6] << 8u) + bRAMarray[7];
                
                //EEPROM_Write();
                
                bI2C_Reg[I2C_COMMAND]  = 0;
                bI2C_Reg[I2C_DATA_MSB] = 0;
                bI2C_Reg[I2C_DATA_LSB] = 0;
                
                bI2C_Reg[I2C_PROXIMITY_SENSITIVITY_MSB] =  g_proximity_sensitivity >> 8u;
                bI2C_Reg[I2C_PROXIMITY_SENSITIVITY_LSB] =  g_proximity_sensitivity & 0xFF;
            }
             
            //WRITE_MODE
            #if(1)
            if(bI2C_Reg[I2C_COMMAND] == WRITE_MODE)
            {
                MODE =  bI2C_Reg[I2C_DATA_MSB];
                
                bI2C_Reg[I2C_COMMAND]  = 0;
                bI2C_Reg[I2C_DATA_MSB] = 0;
                bI2C_Reg[I2C_DATA_LSB] = 0;
            }
            #endif
            
            //CHECK CYPRESS_ENTER_DFU
            //----------------------------------------------------------------------------------
            if(bI2C_Reg[I2C_COMMAND] == CYPRESS_ENTER_DFU) 
            {    
                Bootloadable_Load();
            }
        }
        
        
        //PROXIMITY_PROCESS
        //--------------------------------------------------------------------------------------------------------------------------------------
        if(bActiveCount)
        {
            bActiveCount--;
            
            CapSense_ScanAllWidgets();      //Scan all widgets

            while(CapSense_NOT_BUSY != CapSense_IsBusy())
            {
                CySysPmSleep();    
            }
            
            CapSense_ProcessAllWidgets();
            CapSense_IsWidgetActive(CapSense_PROXIMITY0_WDGT_ID);
            
            //GET_PROXIMITY_DATA
            //----------------------------------------------------------------- 
            g_proximity_currect_raw_count = CapSense_dsRam.snsList.proximity0[CapSense_PROXIMITY0_WDGT_ID].raw[0];
            
            g_proximity_currect_baseline  = CapSense_dsRam.snsList.proximity0[CapSense_PROXIMITY0_WDGT_ID].bsln[0];
            
            g_proximity_diff = CapSense_dsRam.snsList.proximity0[CapSense_PROXIMITY0_WDGT_ID].diff;
            
            
            bI2C_Reg[I2C_CURRENT_RAW_COUNT_MSB] = (g_proximity_currect_raw_count >> 8);
            bI2C_Reg[I2C_CURRENT_RAW_COUNT_LSB] = (g_proximity_currect_raw_count & 0xFF);
            
            bI2C_Reg[I2C_PROXIMITY_BASELINE_MSB] = (g_proximity_currect_baseline >> 8);
            bI2C_Reg[I2C_PROXIMITY_BASELINE_LSB] = (g_proximity_currect_baseline & 0xFF);
            
            bI2C_Reg[I2C_PROXIMITY_DIFF_MSB]    = (g_proximity_diff >> 8);
            bI2C_Reg[I2C_PROXIMITY_DIFF_LSB]    = (g_proximity_diff & 0xFF);
            
            
            #if(0)
            if(MODE != MODE_DIFF_DATA)
            {
                bI2C_Reg[I2C_CURRENT_RAW_COUNT_MSB] = (g_proximity_currect_raw_count >> 8);
                bI2C_Reg[I2C_CURRENT_RAW_COUNT_LSB] = (g_proximity_currect_raw_count & 0xFF);
                
                bI2C_Reg[I2C_PROXIMITY_BASELINE_MSB] = (g_proximity_currect_baseline >> 8);
                bI2C_Reg[I2C_PROXIMITY_BASELINE_LSB] = (g_proximity_currect_baseline & 0xFF);
            }

            
            if(MODE == MODE_DIFF_DATA)
            {
                bI2C_Reg[I2C_THRESHOLD_COUNT_MSB] = (g_proximity_diff >> 8);
                bI2C_Reg[I2C_THRESHOLD_COUNT_LSB] = (g_proximity_diff & 0xFF);
            }
            #endif
            
            
            //UPDATE_PROXIMITY_REFERENCE_DATA
            //----------------------------------------------------------------- 
            if(g_proximity_currect_raw_count > g_proximity_max_raw_count)
            {
                g_proximity_max_raw_count = g_proximity_currect_raw_count;
            }
            
            if(g_proximity_currect_raw_count < g_proximity_min_raw_count)
            {
                g_proximity_min_raw_count = g_proximity_currect_raw_count;
            }
            
            if(g_proximity_diff > g_proximity_max_diff)
            {
                g_proximity_max_diff = g_proximity_diff;
            } 
               
            
            //CHECK_PROXIMITY_ON_OFF
            //----------------------------------------------------------------- 
            //if(CapSense_IsWidgetActive(CapSense_PROXIMITY0_WDGT_ID))
            //if(CapSense_dsRam.snsList.proximity0[CapSense_PROXIMITY0_WDGT_ID].diff >= g_proximity_sensitivity && bPowerOnCheckTh == 0)
            if((g_proximity_diff >= g_proximity_sensitivity) && bPowerOnCheckTh == 0)
            {  
                CyDelay(30u); 
                Pin_interrupt_Write(1);
                   
                //CHECK_TEMP_HIGH_CASE
                //----------------------------------------------------------
                if(g_proximity_max_diff >= 1000u && g_proximity_diff < 500u)
                {
                    #if(1)
                    CapSense_Stop();		    
                    CyDelay(10u);
                    CapSense_Start();		     
                    CyDelay(10u);
                    CapSense_InitializeAllBaselines(); 
                    #endif 
                    
                    g_proximity_max_diff = 0u;
                }  
            } 
                 
            else 
            {        
                //CHECK_POWER_ON_REFERENCE_TH_CASE
                //----------------------------------------------------------
                if(bPowerOnCheckTh == 1)
                {
                    //POWER_ON_REFERENCE_TH_CASE
                    //----------------------------------------------------------
                    if(g_proximity_currect_raw_count >= wDefaultBaseline)
                    {
                        CyDelay(30u);
                        Pin_interrupt_Write(1);
                        
                        if(g_proximity_on_when_power_on_raw_count == 0x0000)
                        {
                            g_proximity_on_when_power_on_raw_count = g_proximity_currect_raw_count;
                        }
                        
                        if(g_proximity_on_when_power_on_baseline == 0x0000)
                        {
                            g_proximity_on_when_power_on_baseline = g_proximity_currect_baseline;
                        }
                
                        //g_proximity_threshold_first_time_check_on_flag = 1u;
                    }

 
                    //POWER_ON_REFERENCE_LESS_TH_CASE
                    //----------------------------------------------------------
                    if(g_proximity_currect_raw_count < wDefaultBaseline)
                    //if( (g_proximity_currect_raw_count < wDefaultBaseline) && g_proximity_threshold_first_time_check_on_flag == 0u)
                    { 
                        #if(0)
                        CapSense_Stop();		    
                        CyDelay(10u);
                        CapSense_Start();		     
                        CyDelay(10u);
                        CapSense_InitializeAllBaselines(); 
                        #endif
                        
                        bPowerOnCheckTh = 0;    
                    }
                    
    
                    if(g_proximity_diff >= PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && g_proximity_on_more_th_when_power_on_raw_count == 0u)
                    {
                        g_proximity_on_more_th_when_power_on_raw_count = g_proximity_currect_raw_count;
                    }
            
                    
                    #if(0)
                    if(g_proximity_diff > PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    {
                        bPowerOnCheckTh = 0; 
                    }
                    #endif
                    

                    #if(1)
                    if(g_proximity_diff > PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    {
                        if(g_proximity_when_power_on_check_candidate_raw_count == 0u)
                        {
                            g_proximity_when_power_on_check_candidate_raw_count = g_proximity_currect_raw_count;
                        }
                        
                        if(g_proximity_when_power_on_check_candidate_raw_count != 0u)
                        {
                            if( ((g_proximity_when_power_on_check_candidate_raw_count >= g_proximity_currect_raw_count) && (g_proximity_when_power_on_check_candidate_raw_count - g_proximity_currect_raw_count) <= 50u) || \
                                ((g_proximity_currect_raw_count >= g_proximity_when_power_on_check_candidate_raw_count) && (g_proximity_currect_raw_count - g_proximity_when_power_on_check_candidate_raw_count) <= 50u) )
                            {
                                g_proximity_when_power_on_diff_on_count = g_proximity_when_power_on_diff_on_count + 1u;
                            }
                            else
                            {
                                if(g_proximity_when_power_on_check_candidate_raw_count != g_proximity_currect_raw_count)
                                {
                                    g_proximity_when_power_on_check_candidate_raw_count = g_proximity_currect_raw_count;
                                }
                        
                                g_proximity_when_power_on_diff_on_count = 0u;
                            }
                        }
 
                        //if(g_proximity_when_power_on_diff_on_count == 200u)     //10 seconds
                        //if(g_proximity_when_power_on_diff_on_count == 100u)     //5 seconds
                        if(g_proximity_when_power_on_diff_on_count == 60u)     //3 seconds
                        {
                            bPowerOnCheckTh = 0;    
                            
                            g_proximity_when_power_on_diff_on_count = 0u;
                        }
                    }   
                    
                    if(g_proximity_diff <= PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH  && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    {
                        g_proximity_when_power_on_diff_on_count = 0u;
                    }
                    #endif

                    
                    #if(0)
                    if(g_proximity_diff > PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    {
                        g_proximity_when_power_on_diff_on_count = g_proximity_when_power_on_diff_on_count + 1u;
                        
                        //if(g_proximity_when_power_on_diff_on_count == 260u)     //10 seconds
                        if(g_proximity_when_power_on_diff_on_count == 130u)     //5 seconds
                        {
                            //Pin_interrupt_Write(1);
                            bPowerOnCheckTh = 0;    
                        }
                    }
 
                    if(g_proximity_diff <= PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    {
                        g_proximity_when_power_on_diff_on_count = 0u;
                    }
                    #endif
                    
                    
                    //if( (g_proximity_max_raw_count - g_proximity_currect_raw_count) > PROXIMITY_ERROR_ON_TO_AUTO_TRACKING_TH && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    //if( (g_proximity_on_when_power_on_baseline > g_proximity_currect_baseline) && (g_proximity_on_when_power_on_baseline - g_proximity_currect_baseline) > 400u && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    if( (g_proximity_on_when_power_on_raw_count > g_proximity_currect_raw_count) && (g_proximity_on_when_power_on_raw_count - g_proximity_currect_raw_count) >= 700u && bPowerOnCheckTh == 1u && wDefaultBaseline != 0u)
                    { 
                        #if(1)
                        CapSense_Stop();		    
                        CyDelay(10u);
                        CapSense_Start();		     
                        CyDelay(10u);
                        CapSense_InitializeAllBaselines(); 
                        #endif
                        
                        g_proximity_max_diff        = 0u;
                        g_proximity_max_raw_count   = 0u;
                        
                        bPowerOnCheckTh = 0;  
                    }
                }
                else
                {
                    Pin_interrupt_Write(0);
                    
                    #if(0)
                    if(g_proximity_on_more_th_when_power_on_raw_count != 0u)
                    {
                        if( ((g_proximity_currect_raw_count > g_proximity_min_raw_count) && (g_proximity_currect_raw_count - g_proximity_min_raw_count) <= 100u) && g_proximity_diff <= 200u)
                        {
                            g_proximity_always_on_flag = 1u;
                            g_proximity_on_more_th_when_power_on_raw_count = 0u;
                        }
                    }
            
                    if(g_proximity_always_on_flag == 1u)
                    {
                        if( (g_proximity_currect_raw_count < g_proximity_on_when_power_on_raw_count))
                        {
                            #if(0) 
                            CapSense_Stop();		    
                            CyDelay(10u);
                            CapSense_Start();		     
                            CyDelay(10u);
                            CapSense_InitializeAllBaselines(); 
                            #endif

                            g_proximity_always_on_flag = 0u;
                        }
                    }

                    
                    if(Pin_interrupt_Read() == 1u && g_proximity_always_on_flag == 0u)
                    {
                        Pin_interrupt_Write(0u); 

                        g_proximity_max_raw_count = 0u;
                    }
                    #endif

                }
            }
        }
        else
        {
            uint8 intState = CyEnterCriticalSection();
            
            bStatus = (EZI2C_EzI2CGetActivity() & EZI2C_EZI2C_STATUS_BUSY);
            
            if (0u == bStatus)
            {
                EZI2C_Sleep();
                CySysPmDeepSleep();  
                CyExitCriticalSection(intState);
                EZI2C_Wakeup();
            }
            else
            {
                bActiveCount = 10;   
                CyExitCriticalSection(intState);   
            }
        }
    }
}
