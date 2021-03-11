#ifndef QSIZEPOLICY_H
#define QSIZEPOLICY_H

inline uint qHash(QSizePolicy key, uint seed = 0) ;

class  QSizePolicy
{
public:
 
     QSizePolicy()  : data(0) { }

     QSizePolicy(Policy horizontal, Policy vertical, ControlType type = DefaultType) 
        : bits{0, 0, quint32(horizontal), quint32(vertical),
               type == DefaultType ? 0 : toControlTypeFieldValue(type), 0, 0, 0}
    {}
    QSizePolicy(Policy horizontal, Policy vertical, ControlType type = DefaultType) 
        : data(0) {
        bits.horPolicy = horizontal;
        bits.verPolicy = vertical;
        setControlType(type);
    }

     Policy horizontalPolicy() const  { return static_cast<Policy>(bits.horPolicy); }
     Policy verticalPolicy() const  { return static_cast<Policy>(bits.verPolicy); }
    ControlType controlType() const ;

     void setHorizontalPolicy(Policy d)  { bits.horPolicy = d; }
     void setVerticalPolicy(Policy d)  { bits.verPolicy = d; }
    void setControlType(ControlType type) ;

     Qt::Orientations expandingDirections() const  {
        return ( (verticalPolicy()   & ExpandFlag) ? Qt::Vertical   : Qt::Orientations() )
             | ( (horizontalPolicy() & ExpandFlag) ? Qt::Horizontal : Qt::Orientations() ) ;
    }

     void setHeightForWidth(bool b)  { bits.hfw = b;  }
     bool hasHeightForWidth() const  { return bits.hfw; }
     void setWidthForHeight(bool b)  { bits.wfh = b;  }
     bool hasWidthForHeight() const  { return bits.wfh; }

     bool operator==(const QSizePolicy& s) const  { return data == s.data; }
     bool operator!=(const QSizePolicy& s) const  { return data != s.data; }

    friend  uint qHash(QSizePolicy key, uint seed)  { return qHash(key.data, seed); }

    operator QVariant() const;

     int horizontalStretch() const  { return static_cast<int>(bits.horStretch); }
     int verticalStretch() const  { return static_cast<int>(bits.verStretch); }
     void setHorizontalStretch(int stretchFactor) { bits.horStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }
     void setVerticalStretch(int stretchFactor) { bits.verStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }

     bool retainSizeWhenHidden() const  { return bits.retainSizeWhenHidden; }
     void setRetainSizeWhenHidden(bool retainSize)  { bits.retainSizeWhenHidden = retainSize; }

     void transpose()  { *this = transposed(); }
    
    QSizePolicy transposed() const 
    {
        return QSizePolicy(bits.transposed());
    }

private:

     QSizePolicy(int i)  : data(i) { }
    struct Bits;
     explicit QSizePolicy(Bits b)  : bits(b) { }

    static  quint32 toControlTypeFieldValue(ControlType type) 
    {
        /*
          The control type is a flag type, with values 0x1, 0x2, 0x4, 0x8, 0x10,
          etc. In memory, we pack it onto the available bits (CTSize) in
          setControlType(), and unpack it here.

          Example:

          0x00000001 maps to 0
          0x00000002 maps to 1
          0x00000004 maps to 2
          0x00000008 maps to 3
          etc.
        */

        return qCountTrailingZeroBits(static_cast<quint32>(type));
    }

    struct Bits {
        quint32 horStretch : 8;
        quint32 verStretch : 8;
        quint32 horPolicy : 4;
        quint32 verPolicy : 4;
        quint32 ctype : 5;
        quint32 hfw : 1;
        quint32 wfh : 1;
        quint32 retainSizeWhenHidden : 1;
        
        Bits transposed() const 
        {
            return {verStretch, // \ swap
                    horStretch, // /
                    verPolicy, // \ swap
                    horPolicy, // /
                    ctype,
                    hfw, // \ don't swap (historic behavior)
                    wfh, // /
                    retainSizeWhenHidden};
        }
    };
    union {
        Bits bits;
        quint32 data;
    };

public:
	enum PolicyFlag {
    GrowFlag = 1,
    ExpandFlag = 2,
    ShrinkFlag = 4,
    IgnoreFlag = 8
	};

	enum Policy {
	    Fixed = 0,
	    Minimum = GrowFlag,
	    Maximum = ShrinkFlag,
	    Preferred = GrowFlag | ShrinkFlag,
	    MinimumExpanding = GrowFlag | ExpandFlag,
	    Expanding = GrowFlag | ShrinkFlag | ExpandFlag,
	    Ignored = ShrinkFlag | GrowFlag | IgnoreFlag
	};

	enum ControlType {
	    DefaultType      = 0x00000001,
	    ButtonBox        = 0x00000002,
	    CheckBox         = 0x00000004,
	    ComboBox         = 0x00000008,
	    Frame            = 0x00000010,
	    GroupBox         = 0x00000020,
	    Label            = 0x00000040,
	    Line             = 0x00000080,
	    LineEdit         = 0x00000100,
	    PushButton       = 0x00000200,
	    RadioButton      = 0x00000400,
	    Slider           = 0x00000800,
	    SpinBox          = 0x00001000,
	    TabWidget        = 0x00002000,
	    ToolButton       = 0x00004000
	};
};

QSizePolicy::ControlType QSizePolicy::controlType() const 
{
    return QSizePolicy::ControlType(1 << bits.ctype);
}


void QSizePolicy::setControlType(ControlType type) 
{
    bits.ctype = toControlTypeFieldValue(type);
}

QSizePolicy::operator QVariant() const
{
    return QVariant(QVariant::SizePolicy, this);
}



#endif // QSIZEPOLICY_H
