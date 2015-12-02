#include "optionsmodel.h"
#include "bitcoinunits.h"
#include <QSettings>

#include "init.h"
#include "walletdb.h"
#include "guiutil.h"

#ifdef USE_NATIVE_I2P
#include "i2p.h"
#include <sstream>

#define I2P_OPTIONS_SECTION_NAME    "I2P"
/*
class ScopeGroupHelper
{
public:
    ScopeGroupHelper(QSettings& settings, const QString& groupName)
        : settings_(settings)
    {
        settings_.beginGroup(groupName);
    }
    ~ScopeGroupHelper()
    {
        settings_.endGroup();
    }

private:
    QSettings& settings_;
};
*/
#endif


OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

bool static ApplyProxySettings()
{
    QSettings settings;
    CService addrProxy(settings.value("addrProxy", "127.0.0.1:9050").toString().toStdString());
    
    if (!settings.value("fUseProxy", false).toBool()) {
        addrProxy = CService();
        return false;
    }
    if (!addrProxy.IsValid())
        return false;
    if (!IsLimited(NET_IPV4))
    SetProxy(NET_IPV4, addrProxy);
    if (!IsLimited(NET_IPV6))
        SetProxy(NET_IPV6, addrProxy);
    SetNameProxy(addrProxy);
    return true;
}

#ifdef USE_NATIVE_I2P
std::string& FormatI2POptionsString(
        std::string& options,
        const std::string& name,
        const std::pair<bool, std::string>& value)
{
    if (value.first)
    {
        if (!options.empty())
            options += " ";
        options += name + "=" + value.second;
    }
    return options;
}

std::string& FormatI2POptionsString(
        std::string& options,
        const std::string& name,
        const std::pair<bool, bool>& value)
{
    if (value.first)
    {
        if (!options.empty())
            options += " ";
        options += name + "=" + (value.second ? "true" : "false");
    }
    return options;
}

std::string& FormatI2POptionsString(
        std::string& options,
        const std::string& name,
        const std::pair<bool, int>& value)
{
    if (value.first)
    {
        if (!options.empty())
            options += " ";
        std::ostringstream oss;
        oss << value.second;
        options += name + "=" + oss.str();
    }
    return options;
}
#endif

void OptionsModel::Init()
{
    QSettings settings;

    // These are Qt-only settings:
    nDisplayUnit = settings.value("nDisplayUnit", BitcoinUnits::SDC).toInt();
    bDisplayAddresses = settings.value("bDisplayAddresses", false).toBool();
    fMinimizeToTray = settings.value("fMinimizeToTray", false).toBool();
    fMinimizeOnClose = settings.value("fMinimizeOnClose", false).toBool();
    nTransactionFee = settings.value("nTransactionFee").toLongLong();
    nReserveBalance = settings.value("nReserveBalance").toLongLong();
    language = settings.value("language", "").toString();
    nRowsPerPage = settings.value("nRowsPerPage", 20).toInt();
    notifications = settings.value("notifications", "*").toStringList();
    visibleTransactions = settings.value("visibleTransactions", "*").toStringList();
    fAutoRingSize = settings.value("fAutoRingSize", false).toBool();
    fAutoRedeemShadow = settings.value("fAutoRedeemShadow", false).toBool();
    nMinRingSize = settings.value("nMinRingSize", 3).toInt();
    nMaxRingSize = settings.value("nMaxRingSize", 200).toInt();

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if (settings.contains("fUseUPnP"))
        SoftSetBoolArg("-upnp", settings.value("fUseUPnP").toBool());
    if (settings.contains("addrProxy") && settings.value("fUseProxy").toBool())
        SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
    if (settings.contains("nSocksVersion") && settings.value("fUseProxy").toBool())
        SoftSetArg("-socks", settings.value("nSocksVersion").toString().toStdString());
    if (settings.contains("detachDB"))
        SoftSetBoolArg("-detachdb", settings.value("detachDB").toBool());
    if (!language.isEmpty())
        SoftSetArg("-lang", language.toStdString());
    if (settings.contains("fStaking"))
        SoftSetBoolArg("-staking", settings.value("fStaking").toBool());
    if (settings.contains("nMinStakeInterval"))
        SoftSetArg("-minstakeinterval", settings.value("nMinStakeInterval").toString().toStdString());
    if (settings.contains("fSecMsgEnabled"))
        SoftSetBoolArg("-nosmsg", !settings.value("fSecMsgEnabled").toBool());
    if (settings.contains("fThinMode"))
        SoftSetBoolArg("-thinmode", settings.value("fThinMode").toBool());
    if (settings.contains("fThinFullIndex"))
        SoftSetBoolArg("-thinfullindex", settings.value("fThinFullIndex").toBool());
    if (settings.contains("nThinIndexWindow"))
        SoftSetArg("-thinindexmax", settings.value("nThinIndexWindow").toString().toStdString());

#ifdef USE_NATIVE_I2P
    //ScopeGroupHelper s(settings, I2P_OPTIONS_SECTION_NAME);

    if (settings.value("I2PuseI2POnly", false).toBool())
    {
        mapArgs["-onlynet"] = NATIVE_I2P_NET_STRING;
        std::vector<std::string>& onlyNets = mapMultiArgs["-onlynet"];
        if (std::find(onlyNets.begin(), onlyNets.end(), NATIVE_I2P_NET_STRING) == onlyNets.end())
            onlyNets.push_back(NATIVE_I2P_NET_STRING);
    }

    if (settings.contains("I2PSAMHost"))
        SoftSetArg(I2P_SAM_HOST_PARAM, settings.value("I2PSAMHost").toString().toStdString());

    if (settings.contains("I2PSAMPort"))
        SoftSetArg(I2P_SAM_PORT_PARAM, settings.value("I2PSAMPort").toString().toStdString());

    if (settings.contains("I2PSessionName"))
        SoftSetArg(I2P_SESSION_NAME_PARAM, settings.value("I2PSessionName").toString().toStdString());

        
    i2pInboundQuantity        = settings.value(SAM_NAME_INBOUND_QUANTITY       , SAM_DEFAULT_INBOUND_QUANTITY       ).toInt();
    i2pInboundLength          = settings.value(SAM_NAME_INBOUND_LENGTH         , SAM_DEFAULT_INBOUND_LENGTH         ).toInt();
    i2pInboundLengthVariance  = settings.value(SAM_NAME_INBOUND_LENGTHVARIANCE , SAM_DEFAULT_INBOUND_LENGTHVARIANCE ).toInt();
    i2pInboundBackupQuantity  = settings.value(SAM_NAME_INBOUND_BACKUPQUANTITY , SAM_DEFAULT_INBOUND_BACKUPQUANTITY ).toInt();
    i2pInboundAllowZeroHop    = settings.value(SAM_NAME_INBOUND_ALLOWZEROHOP   , SAM_DEFAULT_INBOUND_ALLOWZEROHOP   ).toBool();
    i2pInboundIPRestriction   = settings.value(SAM_NAME_INBOUND_IPRESTRICTION  , SAM_DEFAULT_INBOUND_IPRESTRICTION  ).toInt();
    i2pOutboundQuantity       = settings.value(SAM_NAME_OUTBOUND_QUANTITY      , SAM_DEFAULT_OUTBOUND_QUANTITY      ).toInt();
    i2pOutboundLength         = settings.value(SAM_NAME_OUTBOUND_LENGTH        , SAM_DEFAULT_OUTBOUND_LENGTH        ).toInt();
    i2pOutboundLengthVariance = settings.value(SAM_NAME_OUTBOUND_LENGTHVARIANCE, SAM_DEFAULT_OUTBOUND_LENGTHVARIANCE).toInt();
    i2pOutboundBackupQuantity = settings.value(SAM_NAME_OUTBOUND_BACKUPQUANTITY, SAM_DEFAULT_OUTBOUND_BACKUPQUANTITY).toInt();
    i2pOutboundAllowZeroHop   = settings.value(SAM_NAME_OUTBOUND_ALLOWZEROHOP  , SAM_DEFAULT_OUTBOUND_ALLOWZEROHOP  ).toBool();
    i2pOutboundIPRestriction  = settings.value(SAM_NAME_OUTBOUND_IPRESTRICTION , SAM_DEFAULT_OUTBOUND_IPRESTRICTION ).toInt();
    i2pOutboundPriority       = settings.value(SAM_NAME_OUTBOUND_PRIORITY      , SAM_DEFAULT_OUTBOUND_PRIORITY      ).toInt();

    std::string i2pOptionsTemp;
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_QUANTITY       , std::make_pair(settings.contains(SAM_NAME_INBOUND_QUANTITY       ), i2pInboundQuantity));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_LENGTH         , std::make_pair(settings.contains(SAM_NAME_INBOUND_LENGTH         ), i2pInboundLength));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_LENGTHVARIANCE , std::make_pair(settings.contains(SAM_NAME_INBOUND_LENGTHVARIANCE ), i2pInboundLengthVariance));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_BACKUPQUANTITY , std::make_pair(settings.contains(SAM_NAME_INBOUND_BACKUPQUANTITY ), i2pInboundBackupQuantity));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_ALLOWZEROHOP   , std::make_pair(settings.contains(SAM_NAME_INBOUND_ALLOWZEROHOP   ), i2pInboundAllowZeroHop));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_INBOUND_IPRESTRICTION  , std::make_pair(settings.contains(SAM_NAME_INBOUND_IPRESTRICTION  ), i2pInboundIPRestriction));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_QUANTITY      , std::make_pair(settings.contains(SAM_NAME_OUTBOUND_QUANTITY      ), i2pOutboundQuantity));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_LENGTH        , std::make_pair(settings.contains(SAM_NAME_OUTBOUND_LENGTH        ), i2pOutboundLength));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_LENGTHVARIANCE, std::make_pair(settings.contains(SAM_NAME_OUTBOUND_LENGTHVARIANCE), i2pOutboundLengthVariance));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_BACKUPQUANTITY, std::make_pair(settings.contains(SAM_NAME_OUTBOUND_BACKUPQUANTITY), i2pOutboundBackupQuantity));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_ALLOWZEROHOP  , std::make_pair(settings.contains(SAM_NAME_OUTBOUND_ALLOWZEROHOP  ), i2pOutboundAllowZeroHop));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_IPRESTRICTION , std::make_pair(settings.contains(SAM_NAME_OUTBOUND_IPRESTRICTION ), i2pOutboundIPRestriction));
    FormatI2POptionsString(i2pOptionsTemp, SAM_NAME_OUTBOUND_PRIORITY      , std::make_pair(settings.contains(SAM_NAME_OUTBOUND_PRIORITY      ), i2pOutboundPriority));

    if (!i2pOptionsTemp.empty())
        SoftSetArg(I2P_SAM_I2P_OPTIONS_PARAM, i2pOptionsTemp);

    i2pOptions = QString::fromStdString(i2pOptionsTemp);
    
    settings.setValue("b32", QString::fromStdString(I2PSession::Instance().GenerateB32AddressFromDestination(I2PSession::Instance().getMyDestination().pub)));
    
  #endif

}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return GUIUtil::GetStartOnSystemStartup();
        case MinimizeToTray:
            return fMinimizeToTray;
        case MapPortUPnP:
            return settings.value("fUseUPnP", GetBoolArg("-upnp", true));
        case MinimizeOnClose:
            return fMinimizeOnClose;
        case ProxyUse:
            return settings.value("fUseProxy", false);
        case ProxyIP: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QString::fromStdString(proxy.ToStringIP());
            else
                return "";
        }
        case ProxyPort: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(proxy.GetPort());
        }
            break;

        case ProxySocksVersion:
            return settings.value("nSocksVersion", 5);
        case Fee:
            return (qint64) nTransactionFee;
        case ReserveBalance:
            return (qint64) nReserveBalance;
        case DisplayUnit:
            return nDisplayUnit;
        case DisplayAddresses:
            return bDisplayAddresses;
        case DetachDatabases:
            return bitdb.GetDetach();
        case Language:
            return settings.value("language", "");
        case RowsPerPage:
            return nRowsPerPage;
        case AutoRingSize:
            return fAutoRingSize;
        case AutoRedeemShadow:
            return fAutoRedeemShadow;
        case MinRingSize:
            return nMinRingSize;
        case MaxRingSize:
            return nMaxRingSize;
        case Staking:
            return settings.value("fStaking", GetBoolArg("-staking", true)).toBool();
        case MinStakeInterval:
            return nMinStakeInterval;
        case SecureMessaging:
            return fSecMsgEnabled;
        case ThinMode:
            return settings.value("fThinMode",      GetBoolArg("-thinmode",      false)).toBool();
        case ThinFullIndex:
            return settings.value("fThinFullIndex", GetBoolArg("-thinfullindex", false)).toBool();
        case ThinIndexWindow:
            return settings.value("ThinIndexWindow", (qint64) GetArg("-thinindexwindow", 4096)).toInt();
        case Notifications:
            return notifications;
        case VisibleTransactions:
            return visibleTransactions;
#ifdef USE_NATIVE_I2P
        case b32:
        {
            return settings.value("b32", "NONE");
        }
        case I2PUseI2POnly:
        {
            //ScopeGroupHelper s(settings, I2P_OPTIONS_SECTION_NAME);
            bool useI2POnly = false;
            if (mapArgs.count("-onlynet"))
            {
                const std::vector<std::string>& onlyNets = mapMultiArgs["-onlynet"];
                if (std::find(onlyNets.begin(), onlyNets.end(), NATIVE_I2P_NET_STRING) != onlyNets.end())
                    useI2POnly = true;
            }
            return settings.value("I2PuseI2POnly", useI2POnly);
        }
        case I2PSAMHost:
        {
            //ScopeGroupHelper s(settings, I2P_OPTIONS_SECTION_NAME);
            return settings.value("I2PSAMHost", QString::fromStdString(GetArg(I2P_SAM_HOST_PARAM, I2P_SAM_HOST_DEFAULT)));
        }
        case I2PSAMPort:
        {
            //ScopeGroupHelper s(settings, I2P_OPTIONS_SECTION_NAME);
            return settings.value("I2PSAMPort", QString::number((qint64)GetArg(I2P_SAM_PORT_PARAM, I2P_SAM_PORT_DEFAULT)));
        }
        case I2PSessionName:
        {
            //ScopeGroupHelper s(settings, I2P_OPTIONS_SECTION_NAME);
            return settings.value("I2PSessionName", QString::fromStdString(GetArg(I2P_SESSION_NAME_PARAM, I2P_SESSION_NAME_DEFAULT)));
        }
        case I2PInboundQuantity:
            return settings.value(SAM_NAME_INBOUND_QUANTITY, SAM_DEFAULT_INBOUND_QUANTITY).toInt();
        case I2PInboundLength:
            return settings.value(SAM_NAME_INBOUND_LENGTH         , SAM_DEFAULT_INBOUND_LENGTH         ).toInt();
        case I2PInboundLengthVariance:
            return settings.value(SAM_NAME_INBOUND_LENGTHVARIANCE , SAM_DEFAULT_INBOUND_LENGTHVARIANCE ).toInt();
        case I2PInboundBackupQuantity:
            return settings.value(SAM_NAME_INBOUND_BACKUPQUANTITY , SAM_DEFAULT_INBOUND_BACKUPQUANTITY ).toInt();
        case I2PInboundAllowZeroHop:
            return settings.value(SAM_NAME_INBOUND_ALLOWZEROHOP   , SAM_DEFAULT_INBOUND_ALLOWZEROHOP   ).toBool();
        case I2PInboundIPRestriction:
            return settings.value(SAM_NAME_INBOUND_IPRESTRICTION  , SAM_DEFAULT_INBOUND_IPRESTRICTION  ).toInt();
        case I2POutboundQuantity:
            return settings.value(SAM_NAME_OUTBOUND_QUANTITY      , SAM_DEFAULT_OUTBOUND_QUANTITY      ).toInt();
        case I2POutboundLength:
            return settings.value(SAM_NAME_OUTBOUND_LENGTH        , SAM_DEFAULT_OUTBOUND_LENGTH        ).toInt();
        case I2POutboundLengthVariance:
            return settings.value(SAM_NAME_OUTBOUND_LENGTHVARIANCE, SAM_DEFAULT_OUTBOUND_LENGTHVARIANCE).toInt();
        case I2POutboundBackupQuantity:
            return settings.value(SAM_NAME_OUTBOUND_BACKUPQUANTITY, SAM_DEFAULT_OUTBOUND_BACKUPQUANTITY).toInt();
        case I2POutboundAllowZeroHop:
            return settings.value(SAM_NAME_OUTBOUND_ALLOWZEROHOP  , SAM_DEFAULT_OUTBOUND_ALLOWZEROHOP  ).toBool();
        case I2POutboundIPRestriction:
            return settings.value(SAM_NAME_OUTBOUND_IPRESTRICTION , SAM_DEFAULT_OUTBOUND_IPRESTRICTION ).toInt();
        case I2POutboundPriority:
            return settings.value(SAM_NAME_OUTBOUND_PRIORITY      , SAM_DEFAULT_OUTBOUND_PRIORITY      ).toInt();
#endif
        }
    }

    return QVariant();
}

QString OptionsModel::optionIDName(int row)
{
    switch(row)
    {
    case Fee: return "Fee";
    case ReserveBalance: return "ReserveBalance";
    case StartAtStartup: return "StartAtStartup";
    case DetachDatabases: return "DetachDatabases";
    case Staking: return "Staking";
    case MinStakeInterval: return "MinStakeInterval";
    case SecureMessaging: return "SecureMessaging";
    case ThinMode: return "ThinMode";
    case ThinFullIndex: return "ThinFullIndex";
    case ThinIndexWindow: return "ThinIndexWindow";
    case AutoRingSize: return "AutoRingSize";
    case AutoRedeemShadow: return "AutoRedeemShadow";
    case MinRingSize: return "MinRingSize";
    case MaxRingSize: return "MaxRingSize";
    case MapPortUPnP: return "MapPortUPnP";
    case ProxyUse: return "ProxyUse";
    case ProxyIP: return "ProxyIP";
    case ProxyPort: return "ProxyPort";
    case ProxySocksVersion: return "ProxySocksVersion";
    case MinimizeToTray: return "MinimizeToTray";
    case MinimizeOnClose: return "MinimizeOnClose";
    case Language: return "Language";
    case DisplayUnit: return "DisplayUnit";
    case DisplayAddresses: return "DisplayAddresses";
    case RowsPerPage: return "RowsPerPage";
    case Notifications: return "Notifications";
    case VisibleTransactions: return "VisibleTransactions";
#ifdef USE_NATIVE_I2P
    case I2PUseI2POnly: return "I2PUseI2POnly";
    case b32: return "b32";
    case I2PSAMHost: return "I2PSAMHost";
    case I2PSAMPort: return "I2PSAMPort";
    
    case I2PSessionName: return "I2PSessionName";
    
    case I2PInboundQuantity: return "I2PInboundQuantity";
    case I2PInboundLength: return "I2PInboundLength";
    case I2PInboundLengthVariance: return "I2PInboundLengthVariance";
    case I2PInboundBackupQuantity: return "I2PInboundBackupQuantity";
    case I2PInboundAllowZeroHop: return "I2PInboundAllowZeroHop";
    case I2PInboundIPRestriction: return "I2PInboundIPRestriction";
    
    case I2POutboundQuantity: return "I2POutboundQuantity";
    case I2POutboundLength: return "I2POutboundLength";
    case I2POutboundLengthVariance: return "I2POutboundLengthVariance";
    case I2POutboundBackupQuantity: return "I2POutboundBackupQuantity";
    case I2POutboundAllowZeroHop: return "I2POutboundAllowZeroHop";
    case I2POutboundIPRestriction: return "I2POutboundIPRestriction";
    case I2POutboundPriority: return "I2POutboundPriority";
#endif
    }

    return "";
}

int OptionsModel::optionNameID(QString name)
{
    for(int i=0;i<OptionIDRowCount;i++)
        if(optionIDName(i) == name)
            return i;

    return -1;
}

bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP:
            fUseUPnP = value.toBool();
            settings.setValue("fUseUPnP", value.toBool());
            MapPort(value.toBool());
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;
        case ProxyUse:
            settings.setValue("fUseProxy", value.toBool());
            ApplyProxySettings();
            break;
        case ProxyIP: {
            proxyType proxy;
            proxy = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            CNetAddr addr(value.toString().toStdString());
            proxy.SetIP(addr);
            settings.setValue("addrProxy", proxy.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxyPort: {
            proxyType proxy;
            proxy = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            proxy.SetPort(value.toInt());
            settings.setValue("addrProxy", proxy.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
#ifdef USE_NATIVE_I2P
    case I2PUseI2POnly: 
	settings.setValue("I2PUseI2POnly",value.toBool());
	break;
    case I2PSAMHost: 
	settings.setValue("I2PSAMHost",value.toString());
	break;
    case I2PSAMPort: 
	settings.setValue("I2PSAMPort",value.toInt());
	break;
    case I2PSessionName: 
	settings.setValue("I2PSessionName",value.toString());
	break;
    case I2PInboundQuantity: 
	settings.setValue(SAM_NAME_INBOUND_QUANTITY,value.toInt());
	break;
    case I2PInboundLength: 
	settings.setValue(SAM_NAME_INBOUND_LENGTH,value.toInt());
	break;
    case I2PInboundLengthVariance: 
	settings.setValue(SAM_NAME_INBOUND_LENGTHVARIANCE,value.toInt());
	break;
    case I2PInboundBackupQuantity: 
	settings.setValue(SAM_NAME_INBOUND_BACKUPQUANTITY,value.toInt());
	break;
    case I2PInboundAllowZeroHop: 
	settings.setValue(SAM_NAME_INBOUND_ALLOWZEROHOP,value.toBool());
	break;
    case I2PInboundIPRestriction: 
	settings.setValue(SAM_NAME_INBOUND_IPRESTRICTION,value.toInt());
	break;    
    case I2POutboundQuantity: 
	settings.setValue(SAM_NAME_OUTBOUND_QUANTITY,value.toInt());
	break;
    case I2POutboundLength: 
	settings.setValue(SAM_NAME_OUTBOUND_LENGTH,value.toInt());
	break;
    case I2POutboundLengthVariance: 
	settings.setValue(SAM_NAME_OUTBOUND_LENGTHVARIANCE,value.toInt());
	break;
    case I2POutboundBackupQuantity: 
	settings.setValue(SAM_NAME_OUTBOUND_BACKUPQUANTITY,value.toInt());
	break;
    case I2POutboundAllowZeroHop: 
	settings.setValue(SAM_NAME_OUTBOUND_ALLOWZEROHOP,value.toBool());
	break;
    case I2POutboundIPRestriction: 
	settings.setValue(SAM_NAME_OUTBOUND_IPRESTRICTION,value.toInt());
	break;
    case I2POutboundPriority: 
	settings.setValue(SAM_NAME_OUTBOUND_PRIORITY,value.toInt());
	break;
#endif
        case Fee:
            nTransactionFee = value.toLongLong();
            settings.setValue("nTransactionFee", (qint64) nTransactionFee);
            emit transactionFeeChanged(nTransactionFee);
            break;
        case ReserveBalance:
            nReserveBalance = value.toLongLong();
            settings.setValue("nReserveBalance", (qint64) nReserveBalance);
            emit reserveBalanceChanged(nReserveBalance);
            break;
        case DisplayUnit:
            nDisplayUnit = value.toInt();
            settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case DisplayAddresses:
            bDisplayAddresses = value.toBool();
            settings.setValue("bDisplayAddresses", bDisplayAddresses);
            emit displayUnitChanged(settings.value("nDisplayUnit", BitcoinUnits::SDC).toInt());
            break;
        case DetachDatabases: {
            bool fDetachDB = value.toBool();
            bitdb.SetDetach(fDetachDB);
            settings.setValue("detachDB", fDetachDB);
            }
            break;
        case Language:
            settings.setValue("language", value);
            break;
        case RowsPerPage: {
            nRowsPerPage = value.toInt();
            settings.setValue("nRowsPerPage", nRowsPerPage);
            emit rowsPerPageChanged(nRowsPerPage);
            }
            break;
        case Notifications: {
            notifications = value.toStringList();
            settings.setValue("notifications", notifications);
            }
            break;
        case VisibleTransactions: {
            visibleTransactions = value.toStringList();
            settings.setValue("visibleTransactions", visibleTransactions);
            emit visibleTransactionsChanged(visibleTransactions);
            }
            break;
        case AutoRingSize: {
            fAutoRingSize = value.toBool();
            settings.setValue("fAutoRingSize", fAutoRingSize);
            }
            break;
        case AutoRedeemShadow: {
            fAutoRedeemShadow = value.toBool();
            settings.setValue("fAutoRedeemShadow", fAutoRedeemShadow);
            }
            break;
        case MinRingSize: {
            nMinRingSize = value.toInt();
            settings.setValue("nMinRingSize", nMinRingSize);
            }
            break;
        case MaxRingSize: {
            nMaxRingSize = value.toInt();
            settings.setValue("nMaxRingSize", nMaxRingSize);
            }
            break;
        case Staking:
            settings.setValue("fStaking", value.toBool());
            break;
        case MinStakeInterval:
            nMinStakeInterval = value.toInt();
            settings.setValue("nMinStakeInterval", nMinStakeInterval);
            break;
        case ThinMode:
            settings.setValue("fThinMode", value.toBool());
            break;
        case ThinFullIndex:
            settings.setValue("fThinFullIndex", value.toBool());
            break;
        case ThinIndexWindow:
            settings.setValue("fThinIndexWindow", value.toInt());
            break;
        case SecureMessaging: {
            if(value.toBool())
            {
                if(!fSecMsgEnabled)
                    SecureMsgEnable();
            }
            else
                SecureMsgDisable();

            settings.setValue("fSecMsgEnabled", fSecMsgEnabled);
            }
            break;
        default:
            break;
        }
    }
    emit dataChanged(index, index);

    return successful;
}

qint64 OptionsModel::getTransactionFee()
{
    return nTransactionFee;
}

qint64 OptionsModel::getReserveBalance()
{
    return nReserveBalance;
}

bool OptionsModel::getMinimizeToTray()
{
    return fMinimizeToTray;
}

bool OptionsModel::getMinimizeOnClose()
{
    return fMinimizeOnClose;
}

int OptionsModel::getDisplayUnit()
{
    return nDisplayUnit;
}

bool OptionsModel::getDisplayAddresses()
{
    return bDisplayAddresses;
}

int OptionsModel::getRowsPerPage() { return nRowsPerPage; }
QStringList OptionsModel::getNotifications() { return notifications; }
QStringList OptionsModel::getVisibleTransactions() { return visibleTransactions; }
bool OptionsModel::getAutoRingSize() { return fAutoRingSize; }
bool OptionsModel::getAutoRedeemShadow() { return fAutoRedeemShadow; }
int OptionsModel::getMinRingSize() { return nMinRingSize; }
int OptionsModel::getMaxRingSize() { return nMaxRingSize; }
