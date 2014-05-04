/*
    This file is part of the Zero Reserve Plugin for Retroshare.

    Zero Reserve is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Zero Reserve is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Zero Reserve.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "zrtypes.h"
#include "Currency.h"

#include "util/rsthreads.h"

#include <QAbstractItemModel>
#include <QDateTime>
#include <QList>



/**
  An object of this class is either the bid or the ask side of the complete order book.
  */

class OrderBook : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
      A single order
      */
    class Order
    {
    public:
        typedef std::string ID;
        enum OrderType { BID = 0, ASK };
        enum Purpose { NEW = 0,         // new order
                       CANCEL,          // tell everyone I changed my mind
                       FILLED,          // tell everyone this order is gone
                       PARTLY_FILLED,   // everybody update the order book
                       SELL             // tell the buyer through a tunnel that there is a match
                                        // an ask market order will not be published
                                        // bid market orders go to execution right away.
                     };
        Order( bool isMyOrder = false ) : m_isMyOrder( isMyOrder), m_commitment( 0 )
        {}

        ID m_order_id; // hashed from order attributes, a secret and randomness
        bool m_isMyOrder;
        OrderType m_orderType;
        ZR::ZR_Number m_amount;
        ZR::ZR_Number m_price;     // the amount as number for sorting
        Currency::CurrencySymbols m_currency;
        qint64 m_timeStamp;   // no more than 1 order / millisecond
        OrderBook::Order::Purpose m_purpose;
        ZR::ZR_Number m_commitment;    // the amount which is currently processed by active TX
                                       // m_commitment <= m_amount
        ZR::BitcoinAddress m_btcAddr;  // address where those Bitcoins are on

        bool operator == (const Order & other);
        bool operator < ( const Order & other) const;
        void setOrderId();

        static const qint64 timeout;
    };

public:
    typedef QList<Order*>::iterator OrderIterator;
    typedef QList<Order*> OrderList;

    explicit OrderBook();
    virtual ~OrderBook();

    virtual QModelIndex index(int, int, const QModelIndex&) const;
    virtual QModelIndex parent(const QModelIndex&) const;
    virtual int rowCount(const QModelIndex&) const;
    virtual int columnCount(const QModelIndex&) const;
    virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    OrderIterator begin(){ return m_orders.begin(); }
    OrderIterator end() { return m_orders.end(); }

    void setMyOrders( OrderBook * myOrders ){ m_myOrders = myOrders; }

    /** @return ZR::ZR_FINISH if this order has been completed */
    virtual ZR::RetVal processOrder( Order* order );
    virtual ZR::RetVal processMyOrder( Order* order );
    void timeoutOrders();

    void filterOrders(OrderList & filteredOrders , const Currency::CurrencySymbols currencySym);

    /** remove an order from the book
     *  @param order Template for the order to match. Relevant fields: Id, timestamp and currency
     *  @return a pointer to the removed order */
    virtual Order * remove( const std::string & order_id );
    Order * find( const std::string & order_id );

    void beginReset(){ beginResetModel(); }
    void endReset(){ endResetModel(); }

    virtual int addOrder( Order* order );

    mutable RsMutex m_order_mutex;

protected:

    OrderList m_orders;
    OrderList m_filteredOrders;
    Currency::CurrencySymbols m_currency;
    OrderBook * m_myOrders;

protected:
    /** Matches our new order with all others  */
    virtual ZR::RetVal match( Order * ){ return ZR::ZR_FAILURE; }
    /** Matches incoming new order with ours */
    virtual ZR::RetVal matchOther( Order * ){ return ZR::ZR_FAILURE; }
signals:

public slots:
    void setCurrency( const QString & currency );

private:
    static bool compareOrder( const Order * left, const Order * right );
};

#endif // ORDERBOOK_H
