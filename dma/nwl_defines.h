/** @name Buffer Descriptor offsets
 *  USR fields are defined by higher level IP. For example, checksum offload
 *  setup for EMAC type devices. The 1st 8 words are utilized by hardware. Any
 *  words after the 8th are for software use only.
 *  @{
 */
#define DMA_BD_BUFL_STATUS_OFFSET   0x00 /**< Buffer length + status */
#define DMA_BD_USRL_OFFSET          0x04 /**< User logic specific - LSBytes */
#define DMA_BD_USRH_OFFSET          0x08 /**< User logic specific - MSBytes */
#define DMA_BD_CARDA_OFFSET         0x0C /**< Card address */
#define DMA_BD_BUFL_CTRL_OFFSET     0x10 /**< Buffer length + control */
#define DMA_BD_BUFAL_OFFSET         0x14 /**< Buffer address LSBytes */
#define DMA_BD_BUFAH_OFFSET         0x18 /**< Buffer address MSBytes */
#define DMA_BD_NDESC_OFFSET         0x1C /**< Next descriptor pointer */

/* Bit masks for some BD fields */
#define DMA_BD_BUFL_MASK            0x000FFFFF /**< Byte count */
#define DMA_BD_STATUS_MASK          0xFF000000 /**< Status Flags */
#define DMA_BD_CTRL_MASK            0xFF000000 /**< Control Flags */

/* Bit masks for BD control field */
#define DMA_BD_INT_ERROR_MASK       0x02000000 /**< Intr on error */
#define DMA_BD_INT_COMP_MASK        0x01000000 /**< Intr on BD completion */

/* Bit masks for BD status field */
#define DMA_BD_SOP_MASK             0x80000000 /**< Start of packet */
#define DMA_BD_EOP_MASK             0x40000000 /**< End of packet */
#define DMA_BD_ERROR_MASK           0x10000000 /**< BD had error */
#define DMA_BD_USER_HIGH_ZERO_MASK  0x08000000 /**< User High Status zero */
#define DMA_BD_USER_LOW_ZERO_MASK   0x04000000 /**< User Low Status zero */
#define DMA_BD_SHORT_MASK           0x02000000 /**< BD not fully used */
#define DMA_BD_COMP_MASK            0x01000000 /**< BD completed */



#define DMA_BD_MINIMUM_ALIGNMENT    0x40  /**< Minimum byte alignment

/* Common DMA registers */
#define REG_DMA_CTRL_STATUS     0x4000      /**< DMA Common Ctrl & Status */

/* These engine registers are applicable to both S2C and C2S channels. 
 * Register field mask and shift definitions are later in this file.
 */

#define REG_DMA_ENG_CAP         0x00000000  /**< DMA Engine Capabilities */
#define REG_DMA_ENG_CTRL_STATUS 0x00000004  /**< DMA Engine Control */
#define REG_DMA_ENG_NEXT_BD     0x00000008  /**< HW Next desc pointer */
#define REG_SW_NEXT_BD          0x0000000C  /**< SW Next desc pointer */
#define REG_DMA_ENG_LAST_BD     0x00000010  /**< HW Last completed pointer */
#define REG_DMA_ENG_ACTIVE_TIME 0x00000014  /**< DMA Engine Active Time */
#define REG_DMA_ENG_WAIT_TIME   0x00000018  /**< DMA Engine Wait Time */
#define REG_DMA_ENG_COMP_BYTES  0x0000001C  /**< DMA Engine Completed Bytes */

/* Register masks. The following constants define bit locations of various
 * control bits in the registers. For further information on the meaning of 
 * the various bit masks, refer to the hardware spec.
 *
 * Masks have been written assuming HW bits 0-31 correspond to SW bits 0-31 
 */

/** @name Bitmasks of REG_DMA_CTRL_STATUS register.
 * @{
 */
#define DMA_INT_ENABLE              0x00000001  /**< Enable global interrupts */
#define DMA_INT_DISABLE             0x00000000  /**< Disable interrupts */
#define DMA_INT_ACTIVE_MASK         0x00000002  /**< Interrupt active? */
#define DMA_INT_PENDING_MASK        0x00000004  /**< Engine interrupt pending */
#define DMA_INT_MSI_MODE            0x00000008  /**< MSI or Legacy mode? */
#define DMA_USER_INT_ENABLE         0x00000010  /**< Enable user interrupts */
#define DMA_USER_INT_ACTIVE_MASK    0x00000020  /**< Int - user interrupt */
#define DMA_USER_INT_ACK            0x00000020  /**< Acknowledge */
#define DMA_MPS_USED                0x00000700  /**< MPS Used */
#define DMA_MRRS_USED               0x00007000  /**< MRRS Used */
#define DMA_S2C_ENG_INT_VAL         0x00FF0000  /**< IRQ value of 1st 8 S2Cs */
#define DMA_C2S_ENG_INT_VAL         0xFF000000  /**< IRQ value of 1st 8 C2Ss */

/** @name Bitmasks of REG_DMA_ENG_CAP register.
 * @{
 */
/* DMA engine characteristics */
#define DMA_ENG_PRESENT_MASK    0x00000001  /**< DMA engine present? */
#define DMA_ENG_DIRECTION_MASK  0x00000002  /**< DMA engine direction */
#define DMA_ENG_C2S             0x00000002  /**< DMA engine - C2S */
#define DMA_ENG_S2C             0x00000000  /**< DMA engine - S2C */
#define DMA_ENG_TYPE_MASK       0x00000030  /**< DMA engine type */
#define DMA_ENG_BLOCK           0x00000000  /**< DMA engine - Block type */
#define DMA_ENG_PACKET          0x00000010  /**< DMA engine - Packet type */
#define DMA_ENG_NUMBER          0x0000FF00  /**< DMA engine number */
#define DMA_ENG_BD_MAX_BC       0x3F000000  /**< DMA engine max buffer size */


/* Shift constants for selected masks */
#define DMA_ENG_NUMBER_SHIFT        8
#define DMA_ENG_BD_MAX_BC_SHIFT     24

/** @name Bitmasks of REG_DMA_ENG_CTRL_STATUS register.
 * @{
 */
/* Interrupt activity and acknowledgement bits */
#define DMA_ENG_INT_ENABLE          0x00000001  /**< Enable interrupts */
#define DMA_ENG_INT_DISABLE         0x00000000  /**< Disable interrupts */
#define DMA_ENG_INT_ACTIVE_MASK     0x00000002  /**< Interrupt active? */
#define DMA_ENG_INT_ACK             0x00000002  /**< Interrupt ack */
#define DMA_ENG_INT_BDCOMP          0x00000004  /**< Int - BD completion */
#define DMA_ENG_INT_BDCOMP_ACK      0x00000004  /**< Acknowledge */
#define DMA_ENG_INT_ALERR           0x00000008  /**< Int - BD align error */
#define DMA_ENG_INT_ALERR_ACK       0x00000008  /**< Acknowledge */
#define DMA_ENG_INT_FETERR          0x00000010  /**< Int - BD fetch error */
#define DMA_ENG_INT_FETERR_ACK      0x00000010  /**< Acknowledge */
#define DMA_ENG_INT_ABORTERR        0x00000020  /**< Int - DMA abort error */
#define DMA_ENG_INT_ABORTERR_ACK    0x00000020  /**< Acknowledge */
#define DMA_ENG_INT_CHAINEND        0x00000080  /**< Int - BD chain ended */
#define DMA_ENG_INT_CHAINEND_ACK    0x00000080  /**< Acknowledge */

/* DMA engine control */
#define DMA_ENG_ENABLE_MASK         0x00000100  /**< DMA enabled? */
#define DMA_ENG_ENABLE              0x00000100  /**< Enable DMA */
#define DMA_ENG_DISABLE             0x00000000  /**< Disable DMA */
#define DMA_ENG_STATE_MASK          0x00000C00  /**< Current DMA state? */
#define DMA_ENG_RUNNING             0x00000400  /**< DMA running */
#define DMA_ENG_IDLE                0x00000000  /**< DMA idle */
#define DMA_ENG_WAITING             0x00000800  /**< DMA waiting */
#define DMA_ENG_STATE_WAITED        0x00001000  /**< DMA waited earlier */
#define DMA_ENG_WAITED_ACK          0x00001000  /**< Acknowledge */
#define DMA_ENG_USER_RESET          0x00004000  /**< Reset only user logic */
#define DMA_ENG_RESET               0x00008000  /**< Reset DMA engine + user */

#define DMA_ENG_ALLINT_MASK         0x000000BE  /**< To get only int events */

#define DMA_ENGINE_PER_SIZE     0x100   /**< Separation between engine regs */
#define DMA_OFFSET              0       /**< Starting register offset */
                                        /**< Size of DMA engine reg space */
#define DMA_SIZE                (MAX_DMA_ENGINES * DMA_ENGINE_PER_SIZE)


#define TX_CONFIG_ADDRESS   0x9108  /* Reg for controlling TX data */
#define RX_CONFIG_ADDRESS   0x9100  /* Reg for controlling RX pkt generator */
#define PKT_SIZE_ADDRESS    0x9104  /* Reg for programming packet size */
#define STATUS_ADDRESS      0x910C  /* Reg for checking TX pkt checker status */

/* Test start / stop conditions */
#define PKTCHKR             0x00000001  /* Enable TX packet checker */
#define PKTGENR             0x00000001  /* Enable RX packet generator */
#define CHKR_MISMATCH       0x00000001  /* TX checker reported data mismatch */
#define LOOPBACK            0x00000002  /* Enable TX data loopback onto RX */
