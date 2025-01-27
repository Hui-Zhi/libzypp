/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/ResFilters.h
 *
*/
#ifndef ZYPP_RESFILTERS_H
#define ZYPP_RESFILTERS_H

#include <zypp/base/Functional.h>
#include <zypp/Filter.h>
#include <zypp/Resolvable.h>

#include <zypp/PoolItem.h>
#include <zypp/Repository.h>

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  namespace resfilter
  { /////////////////////////////////////////////////////////////////

    /** \defgroup RESFILTERS Filter functors operating on ResObjects.
     * \ingroup g_Functor
     *
     * A simple filter is a function or functor matching the signature:
     * \code
     *   bool simplefilter( ResObject::Ptr );
     * \endcode
     *
     * \note It's not neccessary that your function or functor actually
     * returns \c bool. Anything which is convertible into a \c bool
     * will do;
     *
     * Besides basic filter functors which actually evaluate the
     * \c ResObject (e.g. \ref ByKind, \ref ByName) you may
     * use \ref LOGICALFILTERS to build more complex filters.
     *
     * \code
     * // some 'action' functor, printing and counting
     * // ResObjects.
     * struct PrintAndCount
     * {
     *   PrintAndCount( unsigned & counter_r )
     *   : _counter( counter_r )
     *   {}
     *
     *   bool operator()( ResObject::Ptr p ) const
     *   {
     *     DBG << *p << endl;
     *     ++_counter;
     *     return true;
     *   }
     *
     *   unsigned & _counter;
     * };
     *
     * ResStore store;
     * unsigned counter = 0;
     *
     * // print and count all resolvables
     * store.forEach( PrintAndCount(counter) );
     *
     * // print and count all resolvables named "kernel"
     * counter = 0;
     * store.forEach( ByName("kernel"), PrintAndCount(counter) );
     *
     * // print and count all Packages named "kernel"
     * counter = 0;
     * store.forEach( chain( ByKind(ResKind::package),
     *                       ByName("kernel") ),
     *                PrintAndCount(counter) );
     *
     * // print and count all Packages not named "kernel"
     * counter = 0;
     * store.forEach( chain( ByKind(ResKind::package),
     *                       not_c(ByName("kernel")) ),
     *                PrintAndCount(counter) );
     *
     * // same as above ;)
     * counter = 0;
     * store.forEach( chain( ByKind(ResKind::package),
     *                       chain( not_c(ByName("kernel")),
     *                              PrintAndCount(counter) ) ),
     *                true_c() );
     * \endcode
     *
     * As you can see in the last example there is no difference in using
     * a filter or an action functor, as both have the same signature.
     * A difference of course is the way forEach interprets the returned
     * value.
     *
     * Consequently you can netgate and chain actions as well. Thus
     * <tt>PrintAndCount(counter)</tt> could be
     * <tt>chain(Print(),Count(counter))</tt>, if these functors are
     * provided.
     *
     * \note These functors are not limited to be used with ResStore::forEach.
     * You can use them with std::algorithms as well.
     *
     * \c PrintAndCount is an example how a functor can return data collected
     * during the query. You ca easily write a collector, that takes a
     * <tt>std:list\<ResObject::Ptr\>\&</tt> and fills it with the matches
     * found.
     *
     * But as a rule of thumb, a functor should be lightweight. If you
     * want to get data out, pass references to variables in (and assert
     * these variables live as long as the query lasts). Or use std::ref.
     *
     * Internally all functors are passed by value. Thus it would not help
     * you to create an instance of some collecting functor, and pass it
     * to the query. The query will then fill a copy of your functor, you
     * won't get the data back. (Well, you probabely could, by using
     * boosr::ref).
     *
     * Why functors and not plain functions?
     *
     * You can use plain functions if they don't have to deliver data back to
     * the application.
     * The \c C-style approach is having functions that take a <tt>void * data</tt>
     * as last argument. This \c data pointer is then passed arround and casted
     * up and down.
     *
     * If you look at a functor, you'll see that it contains both, the function
     * to call (its <tt>operator()</tt>) and the data you'd otherwise pass as
     * <tt>void * data</tt>. That's nice and safe.
     *
     * \todo migrate to namespace filter and enhance to support Solvables as well.
    */
    //@{
    ///////////////////////////////////////////////////////////////////
    //
    // Some ResObject attributes
    //
    ///////////////////////////////////////////////////////////////////

    /** */
    template<class TRes>
      inline filter::ByKind byKind()
      { return filter::ByKind( ResTraits<TRes>::kind ); }

    /** Select ResObject by name. */
    struct ByName
    {
      ByName()
      {}

      ByName( const std::string & name_r )
      : _name( name_r )
      {}

      bool operator()( ResObject::constPtr p ) const
      {
        return p->name() == _name;
      }

      std::string _name;
    };

    /** Select ResObject by repository or repository alias. */
    struct ByRepository
    {
      ByRepository( Repository repository_r )
      : _alias( repository_r.info().alias() )
      {}

      ByRepository( const std::string & alias_r )
      : _alias( alias_r )
      {}

      bool operator()( ResObject::constPtr p ) const
      {
        return p->repoInfo().alias() == _alias;
      }

      std::string _alias;
    };

    /** Select ResObject by Edition using \a TCompare functor.
     *
     * Selects ResObject if <tt>TCompare( ResObject->edition(), _edition )</tt>
     * is \c true.
     * \code
     * // use the convenience funktions to create ByEdition:
     *
     * byEdition( someedition ); // selects ResObjects with edition == someedition
     *
     * byEdition( someedition, CompareByGT<Edition>() ) //  edition >  someedition
     * \endcode
    */
    template<class TCompare = CompareByEQ<Edition> >
      struct ByEdition
      {
        ByEdition( const Edition & edition_r, TCompare cmp_r )
        : _edition( edition_r )
        , _cmp( cmp_r )
        {}

        bool operator()( ResObject::constPtr p ) const
        {
          return _cmp( p->edition(), _edition );
        }

        Edition  _edition;
        TCompare _cmp;
      };

    /** */
    template<class TCompare>
      ByEdition<TCompare> byEdition( const Edition & edition_r, TCompare cmp_r )
      { return ByEdition<TCompare>( edition_r, cmp_r ); }

    /** */
    template<class TCompare>
      ByEdition<TCompare> byEdition( const Edition & edition_r )
      { return byEdition( edition_r, TCompare() ); }


    /** Select ResObject by Arch using \a TCompare functor.
     *
     * Selects ResObject if <tt>TCompare( ResObject->arch(), _arch )</tt>
     * is \c true.
     * \code
     * // use the convenience funktions to create ByArch:
     *
     * byArch( somearch ); // selects ResObjects with arch == somearch
     *
     * byArch( somearch, CompareByGT<Arch>() ) //  arch >  somearch
     * \endcode
    */
    template<class TCompare = CompareByEQ<Arch> >
      struct ByArch
      {
        ByArch( const Arch & arch_r, TCompare cmp_r )
        : _arch( arch_r )
        , _cmp( cmp_r )
        {}

        bool operator()( ResObject::constPtr p ) const
        {
          return _cmp( p->arch(), _arch );
        }

        Arch  _arch;
        TCompare _cmp;
      };

    /** */
    template<class TCompare>
      ByArch<TCompare> byArch( const Arch & arch_r, TCompare cmp_r )
      { return ByArch<TCompare>( arch_r, cmp_r ); }

    /** */
    template<class TCompare>
      ByArch<TCompare> byArch( const Arch & arch_r )
      { return byArch( arch_r, TCompare() ); }


    ///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    //
    // Some PoolItem attributes
    //
    ///////////////////////////////////////////////////////////////////

    /** Select PoolItem by installed. */
    struct ByInstalled
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isInstalled();
      }
    };

    /** Select PoolItem by uninstalled. */
    struct ByUninstalled
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isUninstalled();
      }
    };

    /** Select PoolItem by transact. */
    struct ByTransact
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().transacts();
      }
    };

    /** Select PoolItem by lock. */
    struct ByLock
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isLocked();
      }
    };

    /** Select PoolItem by keep. */
    struct ByKeep
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isKept();
      }
    };

    /** PoolItem which is recommended. */
    struct ByRecommended
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isRecommended();
      }
    };

    /** PoolItem which is suggested. */
    struct BySuggested
    {
      bool operator()( const PoolItem & p ) const
      {
        return p.status().isSuggested();
      }
    };

    //@}
    /////////////////////////////////////////////////////////////////
  } // namespace resfilter
  ///////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZYPP_RESFILTERS_H
